# Sentinel Safety System: User Manual

**Version:** 1.0.0  
**Last Updated:** February 3, 2026

---

## 1. Introduction
The **Sentinel Safety System** is an AI-powered surveillance platform designed to monitor industrial and construction sites. It uses high-performance computer vision (TensorRT) to detect personnel entering restricted "Danger Zones" and provides real-time alerts to supervisors.

---

## 2. System Prerequisites
To run the system with full performance, ensure your environment meets the following requirements:
*   **Operating System:** Windows 10/11 or Ubuntu 20.04/22.04.
*   **Hardware:** NVIDIA GPU (GTX 10-series or newer) with at least 4GB VRAM.
*   **Software:**
    *   [Docker Desktop](https://www.docker.com/products/docker-desktop/) (Required for primary deployment).
    *   NVIDIA Container Toolkit (Installed automatically with Docker Desktop on Windows).
    *   NVIDIA Drivers (Version 525+ recommended).

---

## 3. Getting Started

### 3.1 Installation
1.  Download or clone the project repository to your local machine.
2.  Ensure Docker Desktop is running.

### 3.2 Launching the System
*   **Windows:** Double-click `start_system.bat`.
*   **Linux/macOS:** Run `bash start_system.sh` in the terminal.

The system will automatically:
1.  Start an MQTT broker for internal communication.
2.  Build and launch the AI Inference Engine.
3.  Start the Web Dashboard.
4.  Open your default browser to `http://localhost:3001`.

### 3.3 Stopping the System
*   **Windows:** Double-click `stop_system.bat`.
*   **Linux/macOS:** Run `bash stop_system.sh`.

---

## 4. Web Dashboard Navigation

The dashboard is accessible at `http://localhost:3001` and is divided into four main sections:

### 4.1 Home / Live Feed
*   **Tiled View:** Displays up to 4 active camera feeds simultaneously.
*   **System Health:** Real-time status indicators (Online/Offline) for the core AI engine.
*   **Cloud Sync Status:** Indicates if local violation records are successfully synchronizing with the cloud backend.
*   **Violation Counter:** Displays the total number of safety breaches detected in the last 24 hours.

### 4.2 Camera Management
1.  Navigate to the **Cameras** tab.
2.  **Add Camera:** Click the "+" button, enter a friendly name (e.g., "Crane Area") and the RTSP URI of your camera.
3.  **Authentication:** If your camera requires a login, use the format: `rtsp://username:password@ip_address:554/path`.

### 4.3 Safety Zone Editor
1.  Navigate to the **Zones** tab.
2.  Select a camera feed from the dropdown menu.
3.  **Drawing:** Click on the video frame to place points. Connecting 3 or more points creates a polygonal "Danger Zone."
4.  **Editing:** Drag existing points to adjust the zone boundaries.
5.  **Save:** Click **"Save Changes"** to push the configuration to the AI engine instantly.

### 4.4 Violation Logs
*   View a searchable table of all recent safety incidents.
*   Each log entry includes the **Timestamp**, **Camera ID**, **Zone Name**, **Detection Confidence**, and **Cloud Sync Status**.

---

## 5. Advanced Settings

### 5.1 Configuration File (`config.json`)
The system's core parameters are stored in the root `config.json`. Key settings include:
*   `alert_cooldown`: The time (in milliseconds) to wait before sending a repeat alert for the same person (default: 5000ms).
*   `inference_interval`: Number of frames to skip between AI passes (lower is more accurate but heavier on the GPU).
*   `stream_port`: The network port used for the MJPEG video stream (default: 8081).

### 5.2 MQTT Integration
Sentinel can send alerts to external systems (e.g., sirens, mobile apps) via MQTT.
*   **Default Broker:** `localhost:1883`
*   **Topic:** `safety/alerts`
*   **Payload Format:** JSON (includes camera ID, zone name, and timestamp).

### 5.3 System Resilience
*   **Auto-Healing:** The system continuously monitors the AI engine. If a crash or freeze occurs (e.g., due to a GPU driver timeout), the watchdog service will automatically restart the engine within seconds to ensure continuous surveillance.
*   **Robust RTSP:** If a camera feed disconnects, the system employs an exponential backoff strategy to reconnect, preventing network floods while ensuring the feed is restored as soon as possible.

---

## 6. Troubleshooting

| Issue | Potential Cause | Solution |
| :--- | :--- | :--- |
| Dashboard shows "Offline" | Engine container failed to start | Check Docker logs using `docker logs safety-engine`. |
| No Video Feed | Incorrect RTSP URI | Verify the camera URI in a media player like VLC. |
| Slow Detection | Low GPU Resources | Ensure `NVIDIA` is selected as the Docker runtime in settings. |
| Logs are empty | Database Permission | Ensure `safety_violations.db` is writable by the Docker container. |

---

## 7. Support
For technical issues or feature requests, please contact the site administrator or refer to the [Development Journal](journal.md).
