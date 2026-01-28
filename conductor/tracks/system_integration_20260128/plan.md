# Implementation Plan: System Integration & Alerting

## Phase 1: MQTT Client Implementation
- [x] Task: MQTT Client Wrapper [cbc8e1b]
    - [x] Write unit tests for connection, publishing, and disconnection (using a mock or embedded broker if possible, or just interface mocking)
    - [x] Implement `MQTTClient` class using a standard C++ MQTT library (e.g., Paho MQTT C++)
- [ ] Task: Conductor - User Manual Verification 'Phase 1: MQTT Client Implementation' (Protocol in workflow.md)

## Phase 2: Pipeline Orchestration
- [ ] Task: Pipeline Manager Core
    - [ ] Write tests for component initialization and data flow
    - [ ] Implement `PipelineManager` to link `RTSPSource` callbacks to `InferenceEngine` execution
- [ ] Task: Integration of Spatial & Visualizer
    - [ ] Write tests for the full processing chain (Source -> Detect -> Map -> Visualize)
    - [ ] Update `PipelineManager` to include `SpatialMapper` and `Visualizer` steps
- [ ] Task: Conductor - User Manual Verification 'Phase 2: Pipeline Orchestration' (Protocol in workflow.md)

## Phase 3: Application Entry Point & Alerting Logic
- [ ] Task: Alert Generation Logic
    - [ ] Write tests for zone violation checks and alert throttling
    - [ ] Implement logic to trigger MQTT alerts based on `SpatialMapper` results (e.g., "Person in Zone A")
- [ ] Task: Main Application (`main.cpp`)
    - [ ] Implement command-line argument parsing (config path, debug mode)
    - [ ] Wire up the `PipelineManager` and start the main loop
- [ ] Task: Conductor - User Manual Verification 'Phase 3: Application Entry Point & Alerting Logic' (Protocol in workflow.md)
