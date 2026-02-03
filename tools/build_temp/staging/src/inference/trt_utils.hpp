#pragma once

#include <iostream>
#include <vector>
#include <string>

#ifdef ENABLE_TENSORRT
#include <NvInfer.h>
#endif

#ifdef ENABLE_CUDA
#include <cuda_runtime_api.h>

#define checkCudaErrors(val) check((val), #val, __FILE__, __LINE__)
void check(cudaError_t result, char const* const func, const char* const file, int const line);
#endif

namespace trt {

#ifdef ENABLE_TENSORRT
class Logger : public nvinfer1::ILogger {
public:
    Logger(Severity severity = Severity::kINFO) : reportableSeverity(severity) {}

    void log(Severity severity, const char* msg) noexcept override {
        if (severity > reportableSeverity) return;

        switch (severity) {
        case Severity::kINTERNAL_ERROR: std::cerr << "[TRT] INTERNAL_ERROR: "; break;
        case Severity::kERROR: std::cerr << "[TRT] ERROR: "; break;
        case Severity::kWARNING: std::cerr << "[TRT] WARNING: "; break;
        case Severity::kINFO: std::cout << "[TRT] INFO: "; break;
        default: std::cout << "[TRT] VERBOSE: "; break;
        }
        std::cout << msg << std::endl;
    }

    Severity reportableSeverity;
};
#endif

} // namespace trt
