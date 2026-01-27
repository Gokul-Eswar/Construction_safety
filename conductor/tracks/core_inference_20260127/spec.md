# Specification: Core Inference Pipeline & GStreamer Integration

## Overview
This track focuses on building the foundational real-time processing pipeline for the Industrial Geofencing Safety System. It covers video ingestion, high-performance AI inference, and basic spatial logic.

## Functional Requirements
- **RTSP Ingestion:** Robustly ingest RTSP streams using GStreamer.
- **Hardware Acceleration:** Utilize NVIDIA hardware for decoding and inference.
- **AI Inference:** Run YOLOv11-Nano via TensorRT with sub-50ms latency.
- **Person Detection:** Specifically detect 'person' class objects with high confidence.
- **Spatial Logic:** Implement basic 2D homography to map pixel coordinates to floor coordinates.

## Technical Constraints
- **Language:** C++17 or higher.
- **Libraries:** GStreamer 1.x, OpenCV 4.x, TensorRT 10.x+, CUDA 12.x+.
- **Latency Target:** <50ms end-to-end (ingestion to inference result).
- **Quantization:** INT8 or FP16 for TensorRT optimization.

## Success Criteria
- [ ] Successfully display live RTSP feed with <200ms glass-to-glass latency.
- [ ] Person detection running at >30 FPS on target hardware.
- [ ] Validated homography mapping between image points and ground plane.
