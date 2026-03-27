# Technology Stack: Industrial Geofencing Safety System

## Core Development
- **Language:** C++20
  - *Why:* Uncompromising performance and direct memory management required for low-latency video processing.
- **Language:** Python 3.10+
  - *Why:* Primary language for model training (PyTorch/Ultralytics), data preprocessing, and rapid prototyping of CV algorithms before C++ implementation.
- **Build System:** CMake
  - *Why:* Industry standard for C++ project configuration, ensuring cross-platform compatibility and easy dependency management.
- **Compiler:** GCC / Clang
  - *Why:* Mature, highly optimizing compilers with robust support for C++20 features.

## Computer Vision & AI Inference
- **Model:** YOLOv11-Nano
  - *Why:* State-of-the-art accuracy-to-speed ratio. "Nano" variant chosen specifically for maximum frame rate on edge hardware.
- **Inference Engine:** OpenCV DNN (with CUDA backend support)
  - *Why:* Provides universal compatibility (CPU/GPU) via ONNX, simplifying development while offering clear upgrade paths to TensorRT for production.
- **Optimization:** NVIDIA TensorRT (Optional)
  - *Why:* Can be enabled for maximum throughput and lowest latency on dedicated NVIDIA hardware.
- **Image Processing:** OpenCV
  - *Why:* Universal library for pre/post-processing, drawing overlays, and performing geometric transformations (homography).

## Multimedia Pipeline
- **Framework:** GStreamer
  - *Why:* Enterprise-grade multimedia graph framework capable of zero-copy memory transfers and hardware-accelerated decoding/encoding.

## Communication & Integration
- **Messaging:** MQTT (e.g., Eclipse Mosquitto)
  - *Why:* Lightweight, publish/subscribe protocol ideal for IoT. Decouples the detection engine from the alerting systems (dashboards, PLCs).
- **Data Interchange:** JSON
  - *Why:* Human-readable, standard format for alert payloads and configuration.

## Data Persistence
- **Database:** SQLite
  - *Why:* Embedded, serverless, zero-configuration database. Ideal for local logging of high-frequency events without the overhead of a network database.

## Dashboard & Visualization
- **Frontend:** React (TypeScript) with Material UI
  - *Why:* Modern, responsive web interface for viewing live streams (via WebRTC/HLS) and historical alert logs.
- **Backend API:** Node.js / Express
  - *Why:* Efficiently handles WebSocket connections for real-time dashboard updates and serves the frontend.

## Infrastructure & Hardware
- **Target Hardware:** NVIDIA RTX Series GPU
- **OS:** Linux (Ubuntu 22.04/24.04 LTS)
  - *Why:* First-class support for NVIDIA drivers, Docker, and ROS (if needed later).