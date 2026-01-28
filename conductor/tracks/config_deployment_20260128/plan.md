# Implementation Plan: Configuration & Deployment Readiness

## Phase 1: Configuration Logic
- [ ] Task: JSON Dependency & Config Loader
    - [ ] Update CMake to fetch `nlohmann_json`
    - [ ] Write unit tests for parsing sample JSON configs (valid/invalid)
    - [ ] Implement `ConfigLoader` class and `AppConfig` struct
- [ ] Task: Conductor - User Manual Verification 'Phase 1: Configuration Logic' (Protocol in workflow.md)

## Phase 2: Application Integration
- [ ] Task: Refactor Main & Pipeline
    - [ ] Update `PipelineManager` to accept `AppConfig` or specific config structs
    - [ ] Update `main.cpp` to load config file specified via CLI args (defaulting to `config.json`)
    - [ ] Verify end-to-end startup with external config
- [ ] Task: Conductor - User Manual Verification 'Phase 2: Application Integration' (Protocol in workflow.md)
