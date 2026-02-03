# Specification: Native TensorRT Execution

## Context
Currently, the system uses OpenCV's DNN module for inference. While this supports CUDA, it adds overhead and lacks the full optimization capabilities of native TensorRT (e.g., specific layer optimizations, FP16/INT8 calibration). The `ModelLoader` currently contains mock code for TensorRT deserialization.

## Goals
1.  Implement actual TensorRT engine deserialization in `ModelLoader`.
2.  Implement `InferenceEngine` logic to use the TensorRT `IExecutionContext` for inference.
3.  Manage CUDA memory (buffers) for inputs and outputs.
4.  Maintain backward compatibility or a fallback to OpenCV DNN (optional, but good for testing).
5.  Validate performance gain.

## Requirements
-   **Dependencies:** TensorRT C++ API (`NvInfer.h`, `NvOnnxParser.h`), CUDA Runtime (`cuda_runtime_api.h`).
-   **Input:** ONNX model (converted to Engine) or serialized Engine file.
-   **Output:** `std::vector<Detection>` compatible with existing pipeline.

## Architecture Changes
-   **CMakeLists.txt:** Add TensorRT library discovery and linking.
-   **ModelLoader:**
    -   Add `nvinfer1::IRuntime`, `nvinfer1::ICudaEngine`.
    -   Implement `deserializeEngine` using the TRT API.
-   **InferenceEngine:**
    -   Add `nvinfer1::IExecutionContext`.
    -   Manage GPU buffers (`void* buffers[]`).
    -   Implement `preprocess` to upload data to GPU.
    -   Implement `runInference` to execute the context and download results.

## Verification
-   Unit tests mocking TRT calls (if difficult to test natively without GPU in CI) or actual integration tests if environment allows.
-   Comparison of detection output with OpenCV DNN baseline.