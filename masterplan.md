# Master Plan: Sentinel Construction Safety - Production Release

**Goal:** Deliver a fully functional, production-grade industrial safety monitoring system. Eliminate all mocks/simulations and ensure robust, "real-world" operation on NVIDIA edge hardware.

## Phase 1: Elimination of Simulation (The "Real Tech" Upgrade)

The current codebase contains placeholders that prevent true production usage. These must be replaced with functional, robust implementations.

### 1.1 True TensorRT Engine Serialization
- **Issue:** `ModelLoader::saveEngine` writes "TRT-ENGINE-MOCK-DATA" instead of the actual serialized engine.
- **Impact:** The system rebuilds the engine from ONNX on every restart (slow startup, high resource usage).
- **Task:** 
  - Update `ModelLoader::saveEngine` to write the content of `nvinfer1::IHostMemory*` (the plan) to disk.
  - Update `ModelLoader::buildFromOnnx` to call `saveEngine` immediately after building if a cache path is provided.
  - Verify that `deserializeEngine` correctly reads the binary file.

### 1.2 System-Level Process Control via Web UI
- **Issue:** The `POST /api/system/restart` endpoint in `server.js` only reloads the Node.js config, leaving the C++ engine untouched.
- **Impact:** Users cannot recover from camera driver failures or apply deep config changes (like model switching) without SSH access.
- **Task:** 
  - Implement a secure IPC mechanism (or child process call) in `server.js` to trigger a restart of the C++ service.
  - If running in Docker: Use the Docker Socket or a shared "watchdog file" that a supervisor script monitors to restart the container.
  - If running natively: Use `exec('net stop SentinelEngine && net start SentinelEngine')` (Windows) or `systemctl restart sentinel` (Linux).

### 1.3 Verified RTSP Robustness
- **Issue:** Tests rely on mock streams or files. Real RTSP streams have packet loss, jitter, and connection drops.
- **Task:** 
  - Verify `PipelineManager` correctly handles `GST_MESSAGE_EOS` and `GST_MESSAGE_ERROR`.
  - Implement an exponential backoff reconnection strategy for RTSP sources (currently it might just try once or loop tightly).

## Phase 2: Hardening & Reliability

Make the system "Industrial Grade". It must run 24/7 without crashing.

### 2.1 Watchdog & Health Monitoring
- **Requirement:** If the C++ engine hangs (e.g., GPU driver lockup), it must be detected and restarted.
- **Task:**
  - **Internal Watchdog:** The C++ engine should update a "heartbeat" file or timestamp in the DB every 5 seconds.
  - **External Monitor:** A separate script (PowerShell/Bash) or the Node.js backend should check this heartbeat. If stale > 30s, kill and restart the engine.

### 2.2 Data Persistence & Rotation
- **Requirement:** `safety_violations.db` cannot grow indefinitely.
- **Task:**
  - Implement a `DatabaseManager` routine to delete logs older than X days (configurable, default 30 days).
  - Ensure the DB WAL mode is enabled for performance and reliability during power loss.

### 2.3 Secure Configuration
- **Requirement:** Passwords (RTSP credentials, MQTT auth) should not be in plain text if possible, or at least permissions restricted.
- **Task:**
  - Ensure `config.json` is not served via the Web UI (only specific safe fields).
  - Add basic HTTP Basic Auth to the Web UI to prevent unauthorized access on the local network.

## Phase 3: Deployment Engineering

Make it installable and deployable by a technician, not just a developer.

### 3.1 Production Docker Compose
- **Task:** Create `docker-compose.prod.yml`.
  - **Restart Policy:** `restart: unless-stopped` or `always`.
  - **Volumes:** Map `data/` for DB and `models/` for TRT engines to the host filesystem to survive container updates.
  - **Network:** Use `host` networking for best RTSP performance and simpler discovery, or mapped ports.

### 3.2 Automated Installer (Windows/Linux)
- **Task:** Refine `tools/build_installer.ps1` and `setup.ps1`.
  - **Prerequisite Check:** Auto-detect NVIDIA Driver version. Fail if < 520.xx (or whatever TRT 10 requires).
  - **Dependency Install:**
    - **Windows:** Check/Install VC++ Redistributables.
    - **Linux:** Check/Install `nvidia-container-toolkit`.
  - **Service Registration:**
    - **Windows:** Register as a Windows Service (NSSM or generic sc.exe).
    - **Linux:** Create a systemd unit file (`sentinel.service`).

## Phase 4: Verification Strategy

Prove it works.

### 4.1 The "Unplug" Test
- Pull the network cable of a camera -> System should log error, wait, and reconnect when plugged back in.
- Pull the power plug of the PC -> System should boot up and auto-start monitoring without human intervention.

### 4.2 The "Load" Test
- Run 4x 4K Streams simultaneously.
- Verify 30FPS maintenance.
- Verify memory usage is stable (no leaks) over 24 hours.

## Execution Order
1.  **Phase 1** (Immediate - Code Fixes)
2.  **Phase 3** (Packaging - to make testing easier)
3.  **Phase 2** (Hardening - Iterative)
4.  **Phase 4** (Final Validation)
