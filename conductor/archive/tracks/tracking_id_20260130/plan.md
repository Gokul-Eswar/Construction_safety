# Implementation Plan: Advanced Object Tracking

## Phase 1: Tracker Selection & Scaffolding
- [ ] **Research:** Evaluate SORT vs. ByteTrack for C++ implementation ease and performance. (Decision: **SORT** (Simple Online and Realtime Tracking) is likely sufficient and easier to implement from scratch in C++ without complex dependencies).
- [ ] **Data Structure:** Update `Detection` struct in `inference_engine.hpp` to include `int track_id = -1`.

## Phase 2: Kalman Filter & Hungarian Algorithm
- [ ] **Kalman Filter:** Implement a standard Kalman Filter class (or use OpenCV's `cv::KalmanFilter`) to predict object positions.
- [ ] **Hungarian Algorithm:** Implement or import a solver to match predicted tracks with new detections based on IOU (Intersection over Union).
- [ ] **SORT Class:** Create `src/tracking/sort_tracker.cpp` encapsulating the logic.

## Phase 3: Integration
- [ ] **Pipeline Update:** Modify `InferenceEngine::runInference` (or `PipelineManager`) to pass detections through the Tracker before returning.
- [ ] **Visualizer:** Update `Visualizer::drawDetections` to render `det.track_id`.

## Phase 4: Data & Testing
- [ ] **Database:** Ensure `violation_logger` correctly stores the new `track_id` (Schema likely already supports it, verify).
- [ ] **Unit Tests:**
    - Test Kalman Filter prediction.
    - Test ID assignment continuity (Frame 1 -> Frame 2 matches).
    - Test occlusion handling (ID persists after 1-2 missed frames).
