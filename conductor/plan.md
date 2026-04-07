# Implementation Plan: System Operational Restoration

## Objective
Make the "Sentinel" Construction Safety system fully operational. Currently, the C++ Inference Engine fails at startup due to an inability to parse the `yolo11n.onnx` model using the OpenCV DNN fallback, which cascades into a pipeline initialization failure.

## Background & Motivation
The system is in a developmental state following an architecture refactor (as noted in `docs/ARCHITECTURE_FIXES.md`). The web backend is running and MQTT event contracts have been drafted, but the core inference engine is fatally crashing. 
The crash logs indicate: `OpenCV Exception loading ONNX: ... error: (-210:Unsupported format or combination of formats) Failed to parse ONNX model`.
This happens because `ENABLE_TENSORRT` is false (either due to a missing CUDA/TensorRT environment, or building with a non-MSVC toolchain on Windows), forcing a fallback to OpenCV DNN which does not fully support the latest YOLO11 ONNX graph structures without specific export configurations.

## Scope & Impact
This plan covers fixes to the C++ Inference Engine and system lifecycle to ensure robust startup and execution.
- **Inference Engine:** Resolve the ONNX model loading issue.
- **System Lifecycle:** Implement the state machine mentioned in `ARCHITECTURE_FIXES.md` to prevent similar silent cascading failures.

## Proposed Solution

### Phase 1: Fix Model Loading (The Critical Blocker)
1. **Address the OpenCV Fallback Incompatibility:**
   - YOLO11 `.onnx` models typically require specific opsets (e.g., opset 12) or the removal of dynamic shapes/NMS plugins to work in `cv::dnn::readNetFromONNX`.
   - **Action:** Add robust error handling in `model_loader.cpp`. 
   - **Alternative/Better Action:** Create a Python script (`tools/export_yolo_opencv.py` using `ultralytics`) to properly export `yolo11n.pt` to an OpenCV-compatible `yolo11n.onnx` format (e.g., `yolo export model=yolo11n.pt format=onnx opset=12 dynamic=False`).
2. **Improve TensorRT Detection Logging:**
   - Update `CMakeLists.txt` and `main.cpp` to explicitly warn the user if TensorRT is not found, clearly indicating that it is falling back to the much slower, CPU-bound OpenCV backend.

### Phase 2: Implement System Lifecycle (Reliability)
As outlined in `ARCHITECTURE_FIXES.md` (Issue 1):
1. **Create `SystemState` Enum:** 
   - Define `NONE → INITIALIZING → RUNNING → SHUTTING_DOWN → STOPPED`.
2. **Create `SystemLifecycle` Class:**
   - Add state validation at each transition.
   - Implement a health check loop that monitors the `PipelineManager` status every 10 seconds.
   - Detect stale streams and initiate graceful shutdown or restart.
3. **Refactor `main.cpp`:**
   - Wrap the main execution loop in the new `SystemLifecycle` orchestrator.

### Phase 3: Validation and End-to-End Testing
1. Recompile the C++ engine (`build_real` or `build`).
2. Verify the engine starts successfully and connects to the MQTT broker.
3. Verify the Web Dashboard receives telemetry and the test video feed.

## Verification
- [ ] Engine starts without OpenCV DNN exceptions.
- [ ] `engine_native.err.log` shows clean initialization.
- [ ] State transitions (`INITIALIZING` -> `RUNNING`) are logged in `sentinel.log`.
- [ ] Web dashboard shows active streams and receives heartbeat events.