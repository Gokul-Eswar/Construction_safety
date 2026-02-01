# Implementation Plan: Native TensorRT Execution

## Phase 1: Build System & Dependencies
- [x] **Task:** Update `CMakeLists.txt` to find and link TensorRT libraries (`nvinfer`, `nvonnxparser`).
    -   *Why:* Necessary to use the TRT API.
    -   *Files:* `CMakeLists.txt`

## Phase 2: TensorRT Wrapper Classes
- [x] **Task:** Create `TRTLogger` and `TRTCommon` helper structures.
    -   *Why:* TensorRT requires a logger implementation and common CUDA error handling utilities.
    -   *Files:* `src/inference/trt_utils.hpp`, `src/inference/trt_utils.cpp`
- [x] **Task:** Update `ModelLoader` to hold TRT smart pointers and manage runtime.
    -   *Why:* To safely manage the lifetime of `IRuntime`, `ICudaEngine`.
    -   *Files:* `src/inference/model_loader.hpp`, `src/inference/model_loader.cpp`

## Phase 3: Engine Management
- [x] **Task:** Implement `ModelLoader::deserializeEngine` with native TRT API.
    -   *Why:* To load pre-compiled `.engine` files.
    -   *Files:* `src/inference/model_loader.cpp`
- [ ] **Task:** Implement `ModelLoader::buildFromOnnx` with `NvOnnxParser`.
    -   *Why:* To allow converting ONNX models to Engines at runtime if no Engine exists.
    -   *Files:* `src/inference/model_loader.cpp`

## Phase 4: Inference Execution
- [ ] **Task:** Update `InferenceEngine` to manage CUDA buffers.
    -   *Why:* TRT requires explicit memory allocation on GPU.
    -   *Files:* `src/inference/inference_engine.hpp`
- [ ] **Task:** Implement `InferenceEngine::runInference` using TRT Context.
    -   *Why:* To perform the actual forward pass (H2D, Execute, D2H).
    -   *Files:* `src/inference/inference_engine.cpp`

## Phase 5: Verification
- [ ] **Task:** Verify compilation and link against TensorRT libs.
    -   *Why:* Ensure build system finds the libraries.
- [ ] **Task:** Run system and verify detections.
