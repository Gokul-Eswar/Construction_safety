# Implementation Plan: Configuration & Deployment Readiness

## Phase 1: Configuration Logic [checkpoint: f0803ea]
- [x] Task: JSON Dependency & Config Loader [f0803ea]
    - [x] Update CMake to fetch `nlohmann_json`
    - [x] Write unit tests for parsing sample JSON configs (valid/invalid)
    - [x] Implement `ConfigLoader` class and `AppConfig` struct
- [x] Task: Conductor - User Manual Verification 'Phase 1: Configuration Logic' (Protocol in workflow.md) [f0803ea]

## Phase 2: Application Integration [checkpoint: f0803ea]
- [x] Task: Refactor Main & Pipeline [f0803ea]
    - [x] Update `PipelineManager` to accept `AppConfig` or specific config structs
    - [x] Update `main.cpp` to load config file specified via CLI args (defaulting to `config.json`)
    - [x] Verify end-to-end startup with external config
- [x] Task: Conductor - User Manual Verification 'Phase 2: Application Integration' (Protocol in workflow.md) [f0803ea]
