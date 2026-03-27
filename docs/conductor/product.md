# Product Definition: Sentinel Construction Safety

**Status:** Milestone 2 (Production Readiness) - **COMPLETED**

## 1. Vision
A robust, real-time safety monitoring system for industrial construction sites that uses computer vision to detect personnel entering hazardous zones and alerts supervisors immediately.

## 2. Core Features

### ✅ Detection & Inference
-   [x] **Person Detection:** YOLOv11/v8 model to detect "Person" class.
-   [x] **High Performance:** Native TensorRT (C++) implementation for GPU acceleration.
-   [x] **Tracking:** SORT algorithm to persist IDs across frames.

### ✅ Safety Zones
-   [x] **Custom Zones:** Polygonal zones defined via Web UI.
-   [x] **Violation Logic:** Geometric check (Point-in-Polygon) for feet location.
-   [x] **Multi-Zone:** Support for multiple overlapping zones per camera.

### ✅ Alerting
-   [x] **Real-time Alerts:** MQTT messages published instantly on violation.
-   [x] **Throttling:** Smart logic to prevent alert spam (cooldown per person).
-   [x] **Persistence:** Local SQLite database log of all violations.

### ✅ User Interface
-   [x] **Modern Dashboard:** Dark-themed React UI with live video grid.
-   [x] **Multi-Stream:** Support for up to 4 concurrent RTSP camera feeds.
-   [x] **System Control:** Manage cameras, zones, and global settings from the UI.
-   [x] **Live Stream:** Low-latency MJPEG streaming to browser.

### ✅ Deployment
-   [x] **Dockerized:** Full stack (Engine, Web, MQTT) containerized via Docker Compose.
-   [x] **One-Click Start:** simple `.bat` and `.sh` launcher scripts.
-   [x] **Edge Ready:** Compatible with NVIDIA Jetson (ARM64) and x86 GPUs.

## 3. Features 2.0 (New!)
-   [x] **Cloud Sync:** Automatic background synchronization of violation logs to MQTT (`safety/cloud_sync`).
-   [x] **Auth:** Basic Authentication for the Web Dashboard (configured via `config.json`).
-   [x] **Auto-Healing:** Automatic service restart on failure.
-   [x] **Robust RTSP:** Exponential backoff for camera reconnection.

## 4. Future Roadmap (Post-v1.0)
-   [ ] **Analytics:** Heatmaps of worker movement and violation hotspots.
-   [ ] **Email/SMS:** Direct integration (currently handled via MQTT downstream).