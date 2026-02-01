# Implementation Plan: Dockerization & Deployment

## Phase 1: Inference Engine Containerization
- [ ] **Task:** Create `Dockerfile.engine` for the C++ application.
    -   *Why:* To package the core logic and dependencies.
    -   *Details:* Install GStreamer, OpenCV, CMake. Copy source. Build.
    -   *Files:* `Dockerfile.engine`

## Phase 2: Web Interface Containerization
- [ ] **Task:** Create `Dockerfile.web` for the Full Stack Web App.
    -   *Why:* To package the Frontend and Backend.
    -   *Details:* Multi-stage build (Frontend Build -> Backend Run).
    -   *Files:* `Dockerfile.web`

## Phase 3: Orchestration
- [ ] **Task:** Create `docker-compose.yml`.
    -   *Why:* To run the full stack with one command.
    -   *Details:* Define services, networks, volumes, and GPU reservation.
    -   *Files:* `docker-compose.yml`

## Phase 4: Verification
- [ ] **Task:** Verify build and startup.
- [ ] **Task:** Verify inter-container communication (Engine -> MQTT -> Web).
