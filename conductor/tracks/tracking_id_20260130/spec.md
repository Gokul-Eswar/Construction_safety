# Track Specification: Advanced Object Tracking & ID Persistence

## Goal
To transition from frame-by-frame detection (where every frame treats a person as a new object) to a tracking-based system that assigns and maintains unique IDs for individuals across frames.

## Motivation
- **Alert Fatigue:** Without tracking, a person standing in a danger zone triggers an alert *every frame* (e.g., 30 times a second).
- **Logic Gaps:** We cannot currently verify "Time in Zone" or "Entry/Exit" events, only "Presence".
- **Visual Clarity:** Tracking allows us to draw smooth trajectories and stable bounding boxes, rather than jittery detections.

## Core Requirements
1.  **Tracker Implementation:** Integrate a lightweight tracker (e.g., SORT or ByteTrack) into the C++ pipeline.
2.  **ID Persistence:** The system must maintain an object's ID even if detection is missed for a few frames (occlusion handling).
3.  **API Update:** The `Detection` struct/class must now include an `int track_id`.
4.  **Visualizer Update:** Draw the Track ID above the bounding box (e.g., "Person #42").
5.  **Logger Update:** Log the `track_id` to the SQLite database to allow tracing individual paths.

## Constraints
- **Latency:** The tracker must be extremely fast (<2ms per frame) to maintain real-time performance on edge hardware.
- **Complexity:** Avoid heavy deep-learning trackers (like DeepSORT with ReID) if simple IOU-based trackers (SORT) suffice for the camera angle.

## Success Criteria
- [ ] Persons walking through the frame retain the same ID > 90% of the time.
- [ ] Visualizer displays stable IDs (e.g., "ID: 1").
- [ ] SQLite logs show repeated entries for the same `object_id`.
