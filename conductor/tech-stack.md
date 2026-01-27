# Technology Stack: Industrial Geofencing Safety System

## Core Languages & Runtimes
- **C++:** Primary language for the high-performance inference engine and real-time processing pipeline.
- **CUDA:** For GPU-accelerated computations and TensorRT integration.

## Computer Vision & AI
- **YOLOv11-Nano:** The chosen model architecture for lightweight, high-speed object detection.
- **NVIDIA TensorRT:** Used for model optimization (INT8 quantization) to achieve sub-50ms latency on RTX GPUs.
- **OpenCV:** For image manipulation, drawing safety overlays, and homography-based spatial transformations.

## Multimedia & Streaming
- **GStreamer:** Handles robust, low-latency RTSP video ingestion and hardware-accelerated decoding.

## Communication & Integration
- **MQTT:** Low-latency messaging protocol for triggering emergency stops and communicating with industrial IoT devices.

## Infrastructure & Hardware
- **NVIDIA RTX GPU:** Target hardware platform for TensorRT-accelerated inference.
- **Linux (Ubuntu recommended):** Development and deployment environment for optimal GStreamer and NVIDIA driver support.
