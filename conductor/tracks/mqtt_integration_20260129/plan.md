# Implementation Plan: Paho MQTT Integration

## Phase 1: Build System Update
- [x] Add `paho.mqtt.c` as a dependency in `CMakeLists.txt` (Paho C++ depends on C).
- [x] Add `paho.mqtt.cpp` as a dependency in `CMakeLists.txt`.
- [x] Link `paho-mqttpp3` to `main_app` and `unit_tests`.

## Phase 2: Implementation
- [x] Update `src/utils/mqtt_client.hpp` to include Paho headers and member variables.
- [x] Implement `connect`, `publish`, and `disconnect` using Paho in `src/utils/mqtt_client.cpp`.
- [x] Update error handling to catch Paho exceptions.

## Phase 3: Verification
- [x] Update `tests/unit/test_mqtt_client.cpp` if necessary (may need to mock the Paho client or just verify the interface still works).
- [x] Run a local Mosquitto broker (if available) or verify compilation and basic logic.
