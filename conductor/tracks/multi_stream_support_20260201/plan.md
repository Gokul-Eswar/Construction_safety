# Implementation Plan: Multi-Stream Support

## Phase 1: Configuration Refactoring
- [x] **Task:** Update `config.json` schema to use a `streams` array.
    -   *Why:* To allow defining multiple camera sources.
    -   *Files:* `config.json`, `src/utils/config_loader.hpp/cpp`
- [x] **Task:** Update `ConfigLoader` to support the new schema.
    -   *Why:* To load multiple stream configurations.
    -   *Files:* `src/utils/config_loader.cpp`

## Phase 2: Pipeline Orchestration
- [x] **Task:** Refactor `PipelineManager` to manage a vector of stream contexts.
    -   *Why:* To handle multiple parallel pipelines.
    -   *Files:* `src/pipeline/pipeline_manager.hpp`, `src/pipeline/pipeline_manager.cpp`
- [x] **Task:** Ensure thread-safety in shared components (AlertThrottler, ViolationLogger).
    -   *Why:* Multiple threads will be reporting violations simultaneously.
    -   *Files:* `src/utils/violation_logger.cpp`, `src/utils/alert_throttler.cpp`

## Phase 3: Visual Integration
- [ ] **Task:** Implement a Tiled Visualizer.
    -   *Why:* To view all streams at once on the MJPEG feed.
    -   *Files:* `src/utils/visualizer.hpp/cpp`
- [ ] **Task:** Update `MJPEGStreamer` to accept multiple frames and tile them.
    -   *Why:* To provide a unified preview.
    -   *Files:* `src/utils/mjpeg_streamer.cpp`

## Phase 4: Verification
- [ ] **Task:** Test with 2+ mock streams.
- [ ] **Task:** Verify dashboard correctly displays events from all stream IDs.
