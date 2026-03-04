# Track Specification: Paho MQTT Integration

## Goal
Replace the mock MQTT client with a real implementation using the `paho-mqtt-cpp` library to enable actual communication with an MQTT broker.

## Context
The current system uses a `MQTTClient` class that merely logs messages to the console. For production readiness, it must be able to publish alerts to a broker like Mosquitto.

## Requirements
- Use `paho-mqtt-cpp` library.
- Handle asynchronous or synchronous connections (synchronous is fine for the initial implementation).
- Maintain the existing `IMQTTClient` interface to minimize changes in `PipelineManager`.
- Ensure the library is integrated into the CMake build system (via `FetchContent`).

## Constraints
- Must work on Windows (win32).
- Minimize external dependencies if possible, or handle them via CMake.
