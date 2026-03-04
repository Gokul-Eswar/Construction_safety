# Specification: Multi-Stream Support

## Context
The current prototype is hardcoded to handle a single RTSP stream defined in `config.json`. In production environments, multiple cameras are usually required to cover a construction site.

## Goals
1.  Extend the configuration schema to support multiple camera sources.
2.  Refactor `PipelineManager` to instantiate and manage multiple ingestion and inference pipelines.
3.  Ensure efficient resource sharing (e.g., sharing a single `InferenceEngine` instance if possible, or managing multiple instances if GPU memory allows).
4.  Update the `MJPEGStreamer` or Visualizer to display multiple streams (e.g., tiled view or sequential switching).

## Requirements
-   **Configuration:** `streams` array in `config.json` containing `uri`, `id`, and `zones`.
-   **Concurrency:** Each stream should run its ingestion in a separate thread/GStreamer pipeline.
-   **Resource Management:** Ability to limit the number of streams based on hardware capacity.

## Architecture Changes
-   **ConfigLoader:** Update to parse an array of stream configurations.
*   **PipelineManager:** 
    -   Store a map of `StreamID -> StreamContext`.
    -   `StreamContext` includes its own `RTSPSource`, `SortTracker`, and `Visualizer`.
-   **MJPEGStreamer:** Implement a "tiled" view or a way to select which stream to view on the dashboard.

## Verification
-   Simulate multiple RTSP streams (e.g., multiple local files via GStreamer).
-   Verify persistent IDs are unique per stream.
-   Monitor GPU/CPU usage scaling with multiple streams.