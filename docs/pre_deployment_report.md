# Pre-Deployment System Status Report

## 🟢 Ready Components
- **Source Code:** All C++ engine, Web Backend, and Frontend code is present and integrated.
- **Dependencies:** `node_modules` are installed for both backend and frontend.
- **Configuration:** `config.json` is set for a "Simulation Feed" (ideal for localhost demo without a camera).
- **Metrics:** `LatencyLogger` (C++) and `mqttService.js` (Backend) are wired to deliver real-time FPS and latency stats to the dashboard.
- **Assets:** `yolo11n.onnx` model and `safety_violations.db` database are present.

## ⚠️ Action Items / Warnings
1.  **Native Build Failed:** The `build_engine.bat` script failed because the Visual Studio C++ compiler (`cl.exe`) is not in your PATH.
    -   *Impact:* You cannot run `run_demo.bat` (native mode) at this moment.
    -   *Solution:* Use the Docker mode (`tools\\start_system.bat`) which handles the build internally.

2.  **Docker Requirement:**
    -   Ensure **Docker Desktop** is running.
    -   Ensure **NVIDIA Container Toolkit** is installed if you want GPU acceleration.
    -   If you do not have an NVIDIA GPU available to Docker, the system might fail to initialize the TensorRT engine.

## 🚀 How to Run (Recommended)
Run **`tools\\start_system.bat`**.
1.  It will check for Docker.
2.  It will build the engine and web containers.
3.  It will automatically open the dashboard at `http://localhost:3001`.

## 🧪 Verification
The system is designed to provide "real metrics". Once running:
- Look for the **FPS** counter in the top-right of the video feed.
- Check the **System Health** panel for "End-to-End Latency" stats.
