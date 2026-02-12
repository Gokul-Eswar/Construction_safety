# Edge Cases & Mitigation Strategies: Sentinel Safety System

This document outlines specific scenarios where the system may encounter degraded performance or failure, along with technical mitigations.

---

## 1. Computer Vision & Inference Edge Cases

### 1.1 Heavy Occlusion (Partial Visibility)
- **Scenario:** A worker is partially hidden behind a pillar, heavy machinery, or another worker.
- **Impact:** The bounding box may flicker or disappear, leading to missed zone violations.
- **Mitigation:**
    - **Algorithmic:** The **Kalman Filter** in the SORT tracker predicts the worker's position for up to `max_age` frames (currently set to 1) even if detection is lost.
    - **Future Fix:** Implement **Re-Identification (Re-ID)** embeddings to link "person A" before occlusion to "person A" after occlusion based on visual appearance rather than just spatial IOU.

### 1.2 Extreme Lighting (Night/Glare)
- **Scenario:** High-intensity construction floodlights causing glare or total darkness in non-lit areas.
- **Impact:** YOLOv11-Nano may fail to extract meaningful features, leading to false negatives.
- **Mitigation:**
    - **Infrastructure:** Use IR-cut RTSP cameras with active infrared illumination.
    - **Code Fix:** Implement **CLAHE (Contrast Limited Adaptive Histogram Equalization)** in the `preprocess` step in `inference_engine.cpp` to normalize lighting before inference.

### 1.3 Perspective Footprint Error
- **Scenario:** A worker is standing just outside a zone, but their head/torso leans into the zone.
- **Impact:** False positive violation.
- **Mitigation:**
    - **Logic:** The system explicitly uses the **Bottom-Center Point** $(x + w/2, y + h)$ of the bounding box. This ensures we only trigger alerts based on where the worker's feet are touching the ground.

---

## 2. Tracking & Identity Edge Cases

### 2.1 ID Switching (The "Cross-Over" Problem)
- **Scenario:** Two workers walk directly past each other, overlapping in the 2D image plane.
- **Impact:** The tracker may swap their `track_id`s.
- **Mitigation:**
    - **Logic:** Increase the **IOU Threshold** in `sort_tracker.hpp` to be more conservative.
    - **Code Fix:** Incorporate the **Hungarian Algorithm** cost function with a gate that prevents assignments over large spatial distances.

### 2.2 Stationary Target "Ghosting"
- **Scenario:** A worker sits down or stays perfectly still for an extended period.
- **Impact:** Some motion-optimized detectors may drop the detection; Kalman filter variance may collapse.
- **Mitigation:**
    - **Logic:** Implement a "Stationary persistence" logic where if a track was confirmed and hasn't moved, it is kept active even if detection confidence briefly dips below threshold.

---

## 3. Multimedia & Network Edge Cases

### 3.1 RTSP Stream Drop / Timeout
- **Scenario:** Network congestion or camera power failure causes the RTSP feed to stop.
- **Impact:** `PipelineManager` thread hangs or waits indefinitely for frames.
- **Mitigation:**
    - **Code Fix:** Utilize the **GStreamer Bus Watch**. I have documented this in `journal.md`; if `GST_MESSAGE_ERROR` is received, the system must trigger `source->stop()` and a delayed `source->start()` to attempt a reconnection.

### 3.2 High Latency "Drift"
- **Scenario:** The inference engine takes longer than the frame interval (e.g., 40ms for 25 FPS).
- **Impact:** The video feed in the dashboard appears to lag further and further behind real-time.
- **Mitigation:**
    - **Implemented Fix:** The **Leaky Queue** (`leaky=2`) and `appsink drop=true` flags in `rtsp_source.cpp`. This ensures that if the engine is slow, GStreamer simply drops the intermediate frames and hands the *latest* one to the engine next.

---

## 4. System & Hardware Edge Cases

### 4.1 GPU Out-of-Memory (OOM)
- **Scenario:** Multiple 4K streams are added, exceeding the VRAM of the NVIDIA GPU.
- **Impact:** `cudaMalloc` fails, causing the C++ engine to crash.
- **Mitigation:**
    - **Code Fix:** Implement a **Stream Governor**. Before starting a new pipeline, check available VRAM using `cudaMemGetInfo`.
    - **Optimization:** Use `inference_interval` (frame skipping) to reduce the number of concurrent executions on the GPU.

### 4.2 SQLite Database Locking
- **Scenario:** High frequency of violations across 4 streams leads to concurrent write attempts to `safety_violations.db`.
- **Impact:** "Database is locked" error in C++.
- **Mitigation:**
    - **Logic:** Use a **WAL (Write-Ahead Logging)** mode for SQLite.
    - **Code Fix:** In `violation_logger.cpp`, execute `PRAGMA journal_mode=WAL;` upon initialization to allow concurrent readers and a single writer without blocking.

---

## 5. Summary Table for Journal Reference

| Edge Case | Detection Method | Mitigation Logic | Status |
| :--- | :--- | :--- | :--- |
| **Network Jitter** | GStreamer Bus Watch | Latency Buffer (100ms) | Implemented |
| **Alert Spam** | AlertThrottler | Sliding Window Cooldown | Implemented |
| **Frame Lag** | LatencyLogger | Leaky Queue / Frame Drop | Implemented |
| **ID Swap** | SORT IOU Analysis | Hungarian Assignment | Implemented |
| **Camera Shake** | Visual Inspection | Digital Stabilization | Future |
| **Database Lock** | SQLite Error Codes | WAL Journaling Mode | Implemented |
