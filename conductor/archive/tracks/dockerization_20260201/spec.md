# Specification: Dockerization & Deployment

## Context
Currently, the application requires manual installation of numerous dependencies (CUDA, TensorRT, OpenCV, GStreamer, Node.js) on the host machine. This is error-prone and difficult to scale. Dockerization ensures a consistent environment.

## Goals
1.  **Containerize Inference Engine:** Create a Docker image containing the C++ application and all its low-level dependencies (CUDA, TRT, GStreamer).
2.  **Containerize Web Interface:** Create a Docker image for the Node.js backend and React frontend.
3.  **Orchestration:** Use `docker-compose` to manage the lifecycle of the Engine, Web UI, and an MQTT Broker.
4.  **GPU Access:** Configure the containers to access the NVIDIA GPU resources.

## Requirements
-   **Base Image (Engine):** NVIDIA TensorRT base image (e.g., `nvcr.io/nvidia/tensorrt:23.05-py3` or similar Ubuntu 22.04 base with CUDA).
-   **Base Image (Web):** Node.js 18 (Alpine or Slim).
-   **Orchestrator:** Docker Compose v2.
-   **Persistence:** `config.json` and `safety_violations.db` must be mounted as volumes to persist data across container restarts.
-   **Networking:** Containers must communicate via a bridge network; Web UI exposes port 3000 (UI) and 3001 (API); Engine exposes 8081 (Stream).

## Architecture
-   **Service: `engine`**
    -   Builds from `./Dockerfile.engine`
    -   Accesses GPU (`nvidia` runtime).
    -   Mounts `./config.json` and `./safety_violations.db`.
-   **Service: `web`**
    -   Builds from `./Dockerfile.web`
    -   Multi-stage build: Builds React frontend -> Copies to Node backend.
    -   Exposes 3000/3001.
    -   Mounts `./config.json` and `./safety_violations.db` (for reading logs/updating config).
-   **Service: `mqtt`**
    -   Image: `eclipse-mosquitto`
    -   Exposes 1883.

## Verification
-   `docker-compose up --build` successfully starts all services.
-   Web UI is accessible at `localhost:3000`.
-   Live stream is visible (Engine communicating with GPU).
-   Configuration changes in Web UI persist after `docker-compose restart`.