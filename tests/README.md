# Testing Suite - Sentinel Construction Safety

This directory contains the testing infrastructure and unit tests for the Sentinel project.

## Structure

- `unit/`: C++ unit tests using GoogleTest. These tests verify the core logic of the inference engine, spatial mapping, tracking, and utilities.
- `infra/`: Python-based infrastructure tests for environment validation and system-level scaffolding.

## Running Tests

### C++ Unit Tests (GoogleTest)

The C++ tests are integrated into the CMake build system.

1.  **Build the tests:**
    ```bash
    mkdir build && cd build
    cmake ..
    cmake --build . --target unit_tests
    ```

2.  **Run the tests:**
    ```bash
    ctest --output-on-failure
    # OR
    ./bin/unit_tests
    ```

### Python Infrastructure Tests

Ensure you have the required dependencies installed.

```bash
pytest tests/infra
```

## Test Coverage

The following components have dedicated unit tests:
- Inference Engine
- Model Loader
- Pipeline Manager
- Spatial Mapper
- SORT Tracker
- MQTT Client
- Alert Throttler
- Violation Logger
- Config Loader
- RTSP Source
- Visualizer
