#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>
#include <opencv2/opencv.hpp>
#include <condition_variable>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#netinet/in.h>
#include <unistd.h>
typedef int SOCKET;
#define INVALID_SOCKET -1
#endif

class MJPEGStreamer {
public:
    MJPEGStreamer();
    ~MJPEGStreamer();

    bool start(int port);
    void stop();
    void publish(const cv::Mat& frame);

private:
    void listenThread();
    void clientThread(SOCKET client_socket);

    int port_;
    std::atomic<bool> running_;
    SOCKET server_socket_;
    std::thread server_thread_;

    std::mutex frame_mutex_;
    std::condition_variable frame_cond_;
    std::vector<uchar> last_frame_data_;
    uint64_t frame_sequence_ = 0;
};
