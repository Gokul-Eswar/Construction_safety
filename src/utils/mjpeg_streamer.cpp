#include "mjpeg_streamer.hpp"
#include <iostream>

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#endif

MJPEGStreamer::MJPEGStreamer() : port_(0), running_(false), server_socket_(INVALID_SOCKET), frame_sequence_(0) {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

MJPEGStreamer::~MJPEGStreamer() {
    stop();
#ifdef _WIN32
    WSACleanup();
#endif
}

bool MJPEGStreamer::start(int port) {
    port_ = port;
    running_ = true;

    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ == INVALID_SOCKET) return false;

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port_);

    if (bind(server_socket_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        return false;
    }

    if (listen(server_socket_, 5) < 0) return false;

    server_thread_ = std::thread(&MJPEGStreamer::listenThread, this);
    std::cout << "MJPEG Streamer started on port " << port_ << std::endl;
    return true;
}

void MJPEGStreamer::stop() {
    running_ = false;
    frame_cond_.notify_all(); // Wake up any sleeping clients so they can exit
#ifdef _WIN32
    closesocket(server_socket_);
#else
    close(server_socket_);
#endif
    if (server_thread_.joinable()) server_thread_.join();
}

void MJPEGStreamer::publish(const cv::Mat& frame) {
    if (frame.empty()) return;

    std::vector<uchar> buffer;
    // Lower quality slightly for speed if needed, 80 is fine for LAN
    std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY, 80};
    cv::imencode(".jpg", frame, buffer, params);

    {
        std::lock_guard<std::mutex> lock(frame_mutex_);
        last_frame_data_ = std::move(buffer);
        frame_sequence_++;
    }
    frame_cond_.notify_all();
}

void MJPEGStreamer::listenThread() {
    while (running_) {
        sockaddr_in client_addr;
        int client_len = sizeof(client_addr);
        SOCKET client_socket = accept(server_socket_, (struct sockaddr*)&client_addr, &client_len);
        
        if (client_socket != INVALID_SOCKET) {
            std::thread(&MJPEGStreamer::clientThread, this, client_socket).detach();
        }
    }
}

void MJPEGStreamer::clientThread(SOCKET client_socket) {
    // 1. Send HTTP Header
    std::string header = 
        "HTTP/1.0 200 OK\r\n"
        "Server: IndustrialSentinelMJPEG\r\n"
        "Connection: close\r\n"
        "Max-Age: 0\r\n"
        "Expires: 0\r\n"
        "Cache-Control: no-cache, private\r\n"
        "Pragma: no-cache\r\n"
        "Content-Type: multipart/x-mixed-replace; boundary=--boundary\r\n\r\n";
    
    send(client_socket, header.c_str(), header.size(), 0);

    uint64_t last_sent_seq = 0;

    while (running_) {
        std::vector<uchar> frame_to_send;
        
        {
            std::unique_lock<std::mutex> lock(frame_mutex_);
            // Wait for a new frame
            frame_cond_.wait(lock, [this, last_sent_seq] { 
                return !running_ || frame_sequence_ > last_sent_seq; 
            });

            if (!running_) break;

            frame_to_send = last_frame_data_;
            last_sent_seq = frame_sequence_;
        }

        if (frame_to_send.empty()) continue;

        std::string part_header = 
            "--boundary\r\n"
            "Content-Type: image/jpeg\r\n"
            "Content-Length: " + std::to_string(frame_to_send.size()) + "\r\n\r\n";
        
        if (send(client_socket, part_header.c_str(), part_header.size(), 0) <= 0) break;
        if (send(client_socket, (const char*)frame_to_send.data(), frame_to_send.size(), 0) <= 0) break;
        if (send(client_socket, "\r\n", 2, 0) <= 0) break;
    }

#ifdef _WIN32
    closesocket(client_socket);
#else
    close(client_socket);
#endif
}
