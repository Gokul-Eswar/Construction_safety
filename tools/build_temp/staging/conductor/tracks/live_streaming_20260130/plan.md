# Implementation Plan: Live Web Video Streaming

## Phase 1: MJPEG Server Implementation
- [ ] **Library/Approach Selection:** Use a lightweight, single-header MJPEG server implementation or GStreamer's `tcpserversink`. (Decision: A custom C++ `MJPEGStreamer` class using standard sockets is more flexible for serving processed frames).
- [ ] **MJPEGStreamer Class:** Create `src/utils/mjpeg_streamer.hpp/cpp`.
- [ ] **Threading:** The server should run in its own thread to avoid blocking the main pipeline.

## Phase 2: Pipeline Integration
- [ ] **Config Update:** Add `stream_port` (default 8081) to `config.json`.
- [ ] **PipelineManager:** Initialize the streamer and push every Nth processed frame (with overlays) to the server.

## Phase 3: Web Dashboard Update
- [ ] **Frontend Component:** Create a `LiveView` component in React.
- [ ] **Integration:** Place the `LiveView` at the top of the "Monitor" tab.

## Phase 4: Verification
- [ ] Test with multiple browser tabs.
- [ ] Verify overlays are correctly rendered in the browser.
