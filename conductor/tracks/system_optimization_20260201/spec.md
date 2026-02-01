# Specification: System Optimization & Real-Time UX

## Context
Currently, the Web UI polls the backend every 5 seconds for updates, which feels sluggish. Also, running multiple streams at full frame rate can overload the GPU.

## Goals
1.  **Real-Time UI:** Replace polling with **WebSockets** (`socket.io`) to push alerts and stats instantly to the dashboard.
2.  **Inference Throttling:** Implement `frame_skip` logic in the C++ engine to process only every Nth frame, significantly reducing GPU load for multi-stream setups.
3.  **Backend Integration:** Connect the Node.js backend to the MQTT broker to bridge C++ alerts to the Web UI.

## Requirements
-   **Backend:** Install `socket.io` and `mqtt`. Subscribe to system topics.
-   **Frontend:** Install `socket.io-client`. Listen for events.
-   **Engine:** Add `inference_interval` to config. Update `PipelineManager` to skip frames.

## Deliverables
-   Smoother Dashboard with instant violation pop-ups.
-   Lower GPU usage when `inference_interval` > 0.
-   Updated `Dockerfile.web` (dependencies).

## Verification
-   Trigger a violation (mock or real) and see it instantly on the dashboard without refresh.
-   Verify FPS/GPU usage drops when interval is increased.