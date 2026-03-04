#include "trt_utils.hpp"
#include <iostream>

#ifdef ENABLE_CUDA
void check(cudaError_t result, char const* const func, const char* const file, int const line) {
    if (result) {
        std::cerr << "CUDA error at " << file << ":" << line << " code=" << result << " \"" << func << "\" \n";
    }
}

namespace trt {
    size_t getAvailableVRAM() {
        size_t free, total;
        cudaError_t res = cudaMemGetInfo(&free, &total);
        if (res != cudaSuccess) {
            std::cerr << "Failed to get CUDA memory info: " << cudaGetErrorString(res) << std::endl;
            return 0;
        }
        return free;
    }
}
#endif

