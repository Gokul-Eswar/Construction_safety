#include "trt_utils.hpp"
#include <iostream>

#ifdef ENABLE_CUDA
void check(cudaError_t result, char const* const func, const char* const file, int const line) {
    if (result) {
        std::cerr << "CUDA error at " << file << ":" << line << " code=" << result << " \"" << func << "\" \n";
    }
}
#endif

