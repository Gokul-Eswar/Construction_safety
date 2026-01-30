# Track Specification: Live Web Video Streaming

## Goal
To allow users to view the live processed video feed (with AI detections and safety zones) directly on the web dashboard.

## Motivation
- **Situational Awareness:** Safety officers need to see the context of a violation, not just a row in a database.
- **Verification:** Users can visually confirm that safety zones are correctly aligned with the physical environment.
- **User Engagement:** A live video feed makes the dashboard a central monitoring hub.

## Core Requirements
1.  **MJPEG Streamer:** A lightweight HTTP server in C++ that serves the processed OpenCV frames as an MJPEG stream.
2.  **Configurable Port:** The streaming port should be configurable via `config.json`.
3.  **Frontend Integration:** Add a "Live Feed" component to the React dashboard that displays the MJPEG stream.
4.  **Performance:** The streaming should not significantly impact the inference latency.

## Constraints
- **Bandwidth:** MJPEG is simple but bandwidth-heavy. We will limit the streaming resolution or frame rate if needed to ensure smooth performance on the dashboard.
- **Concurrency:** Support at least 2-3 simultaneous dashboard viewers.

## Success Criteria
- [ ] Dashboard displays the live video feed from the C++ engine.
- [ ] Bounding boxes, IDs, and zones are visible in the web stream.
- [ ] Stream latency is < 200ms compared to the raw RTSP feed.
