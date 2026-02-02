# Technical Specification: Sentinel Industrial Geofencing & Safety System

## 1. Abstract
Sentinel is a high-performance, real-time computer vision system designed for autonomous safety monitoring in industrial environments. It leverages a heterogeneous software stack (C++, Python, TensorRT, GStreamer) to detect personnel in hazardous zones with sub-millisecond inference latency. This document details the granular mechanics of the system, from raw byte ingestion to distributed alerting.

---

## 2. Multimedia Pipeline (GStreamer Orchestration)
The system utilizes **GStreamer 1.0** for hardware-accelerated video ingestion. The pipeline is designed for "Zero-Copy" potential and minimal jitter.

### 2.1 Implementation Logic
- **Ingestion:** `rtspsrc` handles the RTSP protocol. A `latency=100` buffer is used to smooth network jitter without introducing significant lag.
- **Decoding:** The pipeline uses `nvv4l2decoder` (on NVIDIA platforms) for hardware-accelerated H.264/H.265 decompression, offloading the CPU.
- **Real-Time Constraint Logic:** To prevent backlog, we employ a **Leaky Queue** strategy:
  - `queue max-size-buffers=1 leaky=2`: Discards the oldest buffer if the processing thread is busy.
  - `appsink max-buffers=1 drop=true`: Ensures that the `on_new_sample` callback always receives the freshest frame.

### 2.2 Data Transition
The raw GStreamer buffer is mapped into an `OpenCV Mat` container. If the format is YUV (I420), it is converted to BGR via `cv::cvtColor` before entering the inference engine.

---

## 3. Computer Vision Subsystem (Inference & Tracking)

### 3.1 Object Detection: YOLOv11-Nano
The system employs **YOLOv11 (Nano variant)**.
- **Model Architecture:** A standard CSP (Cross Stage Partial) backbone with a specialized head for person detection.
- **Optimization:** The model is executed via **NVIDIA TensorRT**.
- **Code Logic:** The `InferenceEngine` class handles:
  1. **Preprocessing:** Resizing to 640x640, normalization (1/255.0), and BGR-to-RGB permutation.
  2. **Execution:** `forward` pass on the GPU.
  3. **Post-processing:** Non-Maximum Suppression (NMS) with configurable confidence and IOU thresholds to eliminate redundant bounding boxes.

### 3.2 Temporal Analysis: SORT Tracking
To maintain identity across frames and reduce false alerts, we implement the **SORT (Simple Online and Realtime Tracking)** algorithm. 

#### 3.2.1 Mathematical Model (Kalman Filter)
The state of each tracked target is modeled by a 7-dimensional vector:
$$x = [u, v, s, r, \dot{u}, \dot{v}, \dot{s}]^T$$
Where:
- $(u, v)$ is the pixel center of the bounding box.
- $s$ is the scale (area).
- $r$ is the aspect ratio.
- $(\dot{u}, \dot{v}, \dot{s})$ are the respective velocities.

#### 3.2.2 Data Association (Hungarian Algorithm)
Data association is performed by calculating an **IOU (Intersection over Union) Distance Matrix** between the predicted Kalman states and newly detected bounding boxes. The Hungarian Algorithm solves the assignment problem to minimize the total cost (1 - IOU), ensuring optimal identity persistence.

---

## 4. Spatial Intelligence & Geofencing

### 4.1 Homography & Mapping
The `SpatialMapper` utility allows for "Image-to-World" coordinate transformation using a $3 \times 3$ matrix $H$:
$$\begin{bmatrix} x_{world} \ y_{world} \ 1 \end{bmatrix} = H \begin{bmatrix} x_{image} \ y_{image} \ 1 \end{bmatrix}$$
This enables the system to calculate personnel proximity to hazards in real-world meters, compensating for perspective distortion.

### 4.2 Geofencing Logic (Point-in-Polygon)
Safety zones are defined as $N$-sided polygons.
- **The Footprint Rule:** Instead of the center of the bounding box, Sentinel uses the **bottom-center point** $(x + w/2, y + h)$ of the detection box to represent the worker's feet.
- **Algorithm:** `cv::pointPolygonTest`. This calculates the shortest distance between a point and a polygon. If the value is $\geq 0$, the person is inside the hazardous zone.

---

## 5. System Orchestration & Concurrency Model

### 5.1 Multi-Threaded Synchronization
Sentinel utilizes an asynchronous, thread-safe architecture:
- **Mutex Isolation:** Each `StreamContext` contains a `frame_mutex` to protect `last_processed_frame`, ensuring the `Tiling Engine` (Thread 3) never reads a partially written buffer from the `Inference Engine` (Thread 2).
- **Atomic Control:** Global state (e.g., `keep_running`) is managed via `std::atomic<bool>` to ensure deterministic shutdown across heterogeneous threads.

### 5.2 Software Tiling Engine
For visualization of $N$ streams, the system performs dynamic spatial compositing:
- **Low-Density (N=1):** Pass-through of raw frame.
- **High-Density (N=2 to 4):** Frames are downsampled to $640 \times 360$ and concatenated using `cv::hconcat` and `cv::vconcat` to form a unified $1280 \times 720$ monitoring grid.

---

## 6. Resilience & Reliability

### 6.1 Stream Recovery logic
The system utilizes the GStreamer **Bus Watch** mechanism. If a `GST_MESSAGE_ERROR` or `GST_MESSAGE_EOS` (End of Stream) is detected (indicating RTSP timeout), the `PipelineManager` triggers an automatic teardown and re-initialization of that specific stream context without interrupting the inference of other active cameras.

---

## 7. Software Quality & Reliability
The codebase adheres to industry-standard C++20 conventions.
- **Static Analysis:** `Clang-Tidy` is integrated via a compilation database (`compile_commands.json`) to enforce `modernize-`, `performance-`, and `bugprone-` checks.
- **Validation:** An automated test suite (Googletest) covers 24 critical paths, including Kalman filter prediction, config parsing, and spatial math.

---

## 8. Summary of System Flow
1. **RTSP Stream** → GStreamer (HW Decode) → **Raw Frame**.
2. **Raw Frame** → TensorRT (YOLOv11) → **Bounding Boxes**.
3. **Boxes** → SORT (Kalman/Hungarian) → **Tracked Persons**.
4. **Tracks** → Spatial Mapper → **Zone Intersection Check**.
5. **Violation** → Alert Throttler → **MQTT Alert & SQLite Log**.
6. **Alert** → Backend Bridge → **WebSocket** → **React Dashboard Notification**.