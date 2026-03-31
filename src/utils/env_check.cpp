#include <iostream>

#ifdef ENABLE_GST
#include <gst/gst.h>
#endif

#ifdef ENABLE_CUDA
#include <cuda_runtime.h>
#include <cuda_runtime_api.h>
#endif

int main(int argc, char *argv[]) {
    // Check GStreamer
#ifdef ENABLE_GST
    GError *err = nullptr;
    gboolean gst_init_success = gst_init_check(&argc, &argv, &err);
    
    if (gst_init_success) {
        std::cout << "GStreamer Initialized: Yes" << "\n";
    } else {
        std::cout << "GStreamer Initialized: No" << "\n";
        if (err) {
            std::cerr << "Error: " << err->message << "\n";
            g_error_free(err);
        }
    }
#else
    std::cout << "GStreamer Initialized: No (Build Disabled)" << "\n";
#endif

    // Check CUDA
#ifdef ENABLE_CUDA
    int deviceCount = 0;
    cudaError_t cudaResult = cudaGetDeviceCount(&deviceCount);

    if (cudaResult == cudaSuccess && deviceCount > 0) {
        std::cout << "CUDA Available: Yes" << "\n";
        std::cout << "CUDA Devices: " << deviceCount << "\n";
    } else {
        std::cout << "CUDA Available: No (Runtime Error)" << "\n";
    }
#else
    std::cout << "CUDA Available: No (Build Disabled)" << "\n";
#endif

    return 0;
}
