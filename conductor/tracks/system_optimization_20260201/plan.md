# Implementation Plan: System Optimization & Real-Time UX

## Phase 1: Inference Optimization
- [x] **Task:** Add `inference_interval` to `config.json` and `AppConfig`.
    -   *Why:* To allow configuration of frame skipping.
    -   *Files:* `config.json`, `src/utils/config_loader.hpp/cpp`
- [x] **Task:** Implement frame skipping logic in `PipelineManager`.
    -   *Why:* To reduce GPU load.
    -   *Files:* `src/pipeline/pipeline_manager.cpp`

## Phase 2: Real-Time Backend
- [x] **Task:** Update `web/backend/package.json` to include `socket.io` and `mqtt`.
    -   *Why:* Required libraries.
- [x] **Task:** Update `server.js` to initialize Socket.IO and subscribe to MQTT.
    -   *Why:* To forward MQTT alerts to the Frontend.
    -   *Files:* `web/backend/server.js`

## Phase 3: Real-Time Frontend
- [x] **Task:** Update `web/frontend/package.json` to include `socket.io-client`.
    -   *Why:* Required library.
- [x] **Task:** Update `Dashboard.tsx` to consume WebSocket events.
    -   *Why:* To display real-time data.
    -   *Files:* `web/frontend/src/pages/Dashboard.tsx`

## Phase 4: Verification
- [x] **Task:** Verify "One-Click" build still works with new dependencies.
