#pragma once
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET -1
#endif

class MJPEGStreamer {
public:
    MJPEGStreamer();
    ~MJPEGStreamer();

    /**
     * @brief Start the MJPEG server on the specified port.
     */
    bool start(int port);

    /**
     * @brief Stop the server.
     */
    void stop();

    /**
     * @brief Push a new frame to all connected clients.
     */
    void publish(const cv::Mat& frame);

private:
    void listenThread();
    void clientThread(SOCKET client_socket);

    int port_;
    std::atomic<bool> running_;
    std::thread server_thread_;
    SOCKET server_socket_;

    std::mutex clients_mutex_;
    std::vector<SOCKET> clients_;
    
    // Most recent encoded frame
    std::vector<uchar> last_frame_data_;
    std::mutex frame_mutex_;
};
