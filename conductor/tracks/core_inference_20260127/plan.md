# Implementation Plan: Core Inference Pipeline & GStreamer Integration

## Phase 1: Foundation & Build System [checkpoint: 94db1f7]
- [x] Task: Project Scaffolding & CMake Configuration [1806819]
    - [ ] Write tests for basic project structure and environment checks
    - [ ] Implement CMakeLists.txt with GStreamer, OpenCV, and CUDA/TensorRT dependencies
- [x] Task: Environment Validation Utility [77b7283]
    - [ ] Write tests for CUDA and GStreamer plugin availability
    - [ ] Implement a utility to check and report system capabilities
- [x] Task: Conductor - User Manual Verification 'Phase 1: Foundation & Build System' (Protocol in workflow.md)

## Phase 2: GStreamer Video Ingestion [checkpoint: 9c735d4]
- [x] Task: RTSP Source Component [97bddbf]
    - [x] Write unit tests for GStreamer pipeline string generation
    - [x] Implement GStreamerRTSPSource class using `rtspsrc` and hardware decoding (`nvv4l2decoder`)
- [x] Task: Frame Ingestion & Buffer Management [97bddbf]
    - [x] Write tests for frame rate and buffer health monitoring
    - [x] Implement frame callback mechanism to pass buffers to the processing engine
- [ ] Task: Conductor - User Manual Verification 'Phase 2: GStreamer Video Ingestion' (Protocol in workflow.md)

## Phase 3: TensorRT Inference Engine
- [ ] Task: TensorRT Model Loader & Optimizer
    - [ ] Write tests for model file existence and engine serialization/deserialization
    - [ ] Implement YOLOv11 TensorRT engine loader (supporting ONNX to TRT conversion)
- [ ] Task: Inference Pipeline Implementation
    - [ ] Write unit tests for pre-processing (resize, normalization) and post-processing (NMS)
    - [ ] Implement `InferenceEngine` class to execute TensorRT context on GPU
- [ ] Task: Conductor - User Manual Verification 'Phase 3: TensorRT Inference Engine' (Protocol in workflow.md)

## Phase 4: Spatial Mapping & Visualization
- [ ] Task: Homography Transformation Logic
    - [ ] Write tests for coordinate transformation accuracy against known points
    - [ ] Implement `SpatialMapper` class using OpenCV `findHomography` and `perspectiveTransform`
- [ ] Task: Detection Overlay & Result Visualization
    - [ ] Write tests for overlay rendering performance
    - [ ] Implement visualization layer to draw bounding boxes and safety zones on the video frame
- [ ] Task: Conductor - User Manual Verification 'Phase 4: Spatial Mapping & Visualization' (Protocol in workflow.md)
