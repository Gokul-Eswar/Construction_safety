# Track Specification: Configuration & Deployment Readiness

## Goal
To transition the application from a hardcoded prototype to a configurable, deployable system. This involves decoupling configuration parameters (RTSP URLs, model paths, safety zones, MQTT settings) from the code and managing them via external JSON files.

## Scope
1.  **Configuration Loader:** A module to parse JSON configuration files.
2.  **Application Refactoring:** Updating `main.cpp` and `PipelineManager` to accept configuration objects.
3.  **Build Hardening:** Ensuring `CMakeLists.txt` handles dependencies (like `nlohmann_json`) robustly.

## Components
-   `ConfigLoader`: Class to read and validate `config.json`.
-   `AppConfig`: Struct/Class holding runtime settings.
-   `main.cpp`: Updated to load config at startup.

## Success Criteria
-   Application starts using values from `config.json`.
-   Changing `config.json` updates behavior (e.g., zones, MQTT topic) without recompilation.
-   Unit tests verify correct parsing of valid and invalid config files.
