# Track Specification: System Integration & Alerting

## Goal
To integrate the individual components developed in the previous track (RTSP Source, Inference Engine, Spatial Mapper, Visualizer) into a cohesive, running application. Additionally, implement the critical safety alerting mechanism using MQTT to trigger hardware stops or external alarms.

## Scope
1.  **Pipeline Manager:** A central class to manage the lifecycle of all components and the flow of data (frames) between them.
2.  **Main Application:** The entry point (`main.cpp`) that configures and starts the system.
3.  **MQTT Client:** A module to publish JSON-formatted alerts to an MQTT broker when a person is detected in a safety zone.
4.  **End-to-End Testing:** Verification of the complete flow from video ingestion to alert generation.

## Components
-   `PipelineManager`: Orchestrator.
-   `MQTTClient`: Handles network communication for alerts.
-   `main.cpp`: CLI entry point.

## Success Criteria
-   The application compiles and runs.
-   Video is ingested, processed, and visualized (even if mocked/test source).
-   Detections in defined zones trigger MQTT messages.
-   System handles graceful shutdown.
