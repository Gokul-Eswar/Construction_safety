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
    GError *err = NULL;
    gboolean gst_init_success = gst_init_check(&argc, &argv, &err);
    
    if (gst_init_success) {
        std::cout << "GStreamer Initialized: Yes" << std::endl;
    } else {
        std::cout << "GStreamer Initialized: No" << std::endl;
        if (err) {
            std::cerr << "Error: " << err->message << std::endl;
            g_error_free(err);
        }
    }
#else
    std::cout << "GStreamer Initialized: No (Build Disabled)" << std::endl;
#endif

    // Check CUDA
#ifdef ENABLE_CUDA
    int deviceCount = 0;
    cudaError_t cudaResult = cudaGetDeviceCount(&deviceCount);

    if (cudaResult == cudaSuccess && deviceCount > 0) {
        std::cout << "CUDA Available: Yes" << std::endl;
        std::cout << "CUDA Devices: " << deviceCount << std::endl;
    } else {
        std::cout << "CUDA Available: No (Runtime Error)" << std::endl;
    }
#else
    std::cout << "CUDA Available: No (Build Disabled)" << std::endl;
#endif

    return 0;
}