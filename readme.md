# Sentinel: Construction Safety Inference System

**A high-performance, real-time AI system for monitoring industrial safety zones.**

Sentinel detects personnel in hazardous areas using advanced computer vision and instantly alerts safety supervisors. It features native TensorRT inference, multi-camera support, and a modern web dashboard for complete system management.

---

## 🚀 Quick Start (One-Click)

**Prerequisites:**
1.  **Docker Desktop** (Installed and Running).
2.  **NVIDIA GPU** (Recommended for performance).

**To Start:**
1.  Double-click **`start_system.bat`** (Windows) or run `./start_system.sh` (Linux).
2.  Wait for the system to initialize (first run takes a few minutes to build).
3.  The **Web Dashboard** will open automatically in your browser (`http://localhost:3001`).

**To Stop:**
1.  Double-click **`stop_system.bat`**.

---

## ✨ Key Features

-   **👁️ Multi-Camera Surveillance:** Monitor up to 4 RTSP feeds simultaneously in a unified grid view.
-   **🧠 High-Performance AI:** Uses **TensorRT** (C++ Native) for sub-millisecond inference on NVIDIA GPUs.
-   **⏱️ Low-Latency Optimization:** GStreamer pipeline tuned for zero-latency frame delivery and real-time responsiveness.
-   **📈 Performance Monitoring:** Integrated **LatencyLogger** for tracking end-to-end processing times and pipeline health.
-   **🚧 Interactive Zone Editor:** Draw custom safety zones directly on the video feed via the Web UI.
-   **⚡ Real-Time Alerting:** Detects zone violations instantly and throttles alerts to prevent fatigue.
-   **📊 Modern Dashboard:** Dark-themed UI with real-time health metrics, violation logs, and camera management.
-   **🐳 Dockerized Deployment:** "Write once, run anywhere" architecture for easy deployment on Edge devices (Jetson/x86).

## ✨ Features 2.0 (New!)

-   **☁️ Cloud Sync:** Automatic background synchronization of violation records to the cloud (via MQTT `safety/cloud_sync`).
-   **🔄 Auto-Healing:** The system detects engine crashes or freezes and automatically restarts within seconds, ensuring 24/7 reliability.
-   **📜 Historical Logs:** A dedicated "Violation Logs" page to browse, filter, and audit past safety incidents.
-   **📡 Robust RTSP:** Exponential backoff reconnection strategy to handle unstable camera feeds gracefully.
-   **🚀 Optimized Web:** Gzip compression and improved UI feedback for a snappier dashboard experience.

---

## 🖥️ Web Interface Guide

### 1. Dashboard
-   View the live **Tiled Feed** of all active cameras.
-   Monitor system health (Online/Offline status) and today's violation statistics.
-   **New:** View Cloud Sync status and real-time latency metrics.

### 2. Violation Logs
-   Browse a paginated history of all detected safety violations.
-   View the synchronization status of each record (Uploaded/Pending).

### 3. Cameras (Stream Manager)
-   **Add Camera:** Click "Add Camera" and enter the RTSP URI (e.g., `rtsp://user:pass@ip:554/feed`).
-   **Edit/Delete:** Manage your existing camera inventory.

### 3. Safety Zones
-   Select a camera from the dropdown.
-   Click on the canvas to draw a polygonal **Safety Zone**.
-   Click **"Save Changes"** to apply the zone immediately.

### 4. Settings
-   **Global:** Configure the AI Model path and Alert Cooldown (ms).
-   **MQTT:** Configure your MQTT Broker connection details for external alerting.
-   **System:** Restart the core service to apply major configuration changes.

---

## 🛠️ Developer Guide (Manual Build)

If you wish to develop or modify the source code without Docker:

### Prerequisites
-   CMake 3.18+
-   GStreamer 1.0 (Development Libraries)
-   OpenCV 4.x
-   CUDA Toolkit & TensorRT (Optional, for GPU acceleration)
-   Node.js 18+ (For Web UI)
-   **Windows Only:** Visual Studio Build Tools (C++ Desktop Development workload) - Required for `cl.exe`.

### Building the Engine (C++)
> **Note:** On Windows, ensure you run these commands from the "x64 Native Tools Command Prompt for VS 20xx" or ensure `cl.exe` is in your system PATH.

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

### Building the Web UI
```bash
cd web/frontend
npm install && npm run build
cd ../backend
npm install
node server.js
```

---

## 🏗️ Architecture

- **Inference Engine:** C++17, TensorRT, GStreamer, OpenCV.

- **Web Backend:** Node.js, Express, SQLite3.

- **Web Frontend:** React, TypeScript, Material UI (MUI v5).

- **Communication:** MQTT (Alerts), MJPEG (Video Stream).



---



## 🧪 Rigorous Testing



Sentinel includes a comprehensive suite of rigorous tests designed for industrial stability:



- **STRESS TEST:** Verifies multi-stream stability by running 4+ simultaneous feeds to monitor GPU memory drift and thread contention.

- **RESILIENCE TEST:** Simulates network failures (RTSP drops) and verifies the exponential backoff reconnection strategy.

- **GEOMETRIC ACCURACY:** Validates the "Bottom-Center Point" logic to ensure alerts are triggered only by feet-on-ground contact, preventing false positives from leaning personnel.

- **CI INTEGRATION:** All core components are verified via **Googletest** (C++) and **Pytest** (System E2E).



To run tests:

```bash

./run_tests.bat

```



---



## 📚 References





### Project Documentation

- [**User Manual**](user_manual.md) - Comprehensive guide for operators and admins.

- [**Project Checklist**](checklist.md) - Tracking features and progress.

- [**Edge Cases**](edge_cases.md) - Known limitations and handled scenarios.

- [**Development Journal**](journal.md) - Daily logs and architectural decisions.

- [**Web README**](web/README.md) - Details on the Dashboard and Backend.



### Conductor (Project Management)

- [**Conductor Hub**](conductor/index.md) - Central entry point for project structure.

- [**Product Definition**](conductor/product.md) - Vision and core requirements.

- [**System Architecture**](conductor/architecture.md) - High-level system design.

- [**Tech Stack**](conductor/tech-stack.md) - Detailed breakdown of libraries and tools.

- [**Workflow**](conductor/workflow.md) - Standards for development and PRs.

- [**Product Guidelines**](conductor/product-guidelines.md) - UX and code quality principles.

- [**Tracks Registry**](conductor/tracks.md) - Status of all feature development tracks.


