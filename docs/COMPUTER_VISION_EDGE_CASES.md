# Computer Vision Edge Cases - Implementation Plan

**Date:** April 5, 2026  
**Version:** 1.0 (Planning Phase)  
**Scope:** Heavy occlusion, extreme lighting, perspective errors in detection

---

## Executive Summary

Four critical computer vision edge cases reduce system robustness:
1. **Heavy Occlusion** — Kalman filter timeout too aggressive (max_age=1 frame)
2. **Extreme Lighting** — No preprocessing; YOLO struggles with glare/darkness
3. **Perspective Errors** — Zone checking doesn't account for 3D footprint mapping
4. **Configuration** — Parameters hardcoded; no runtime tuning

**Impact:** False negatives (missed violations), false positives (incorrect alerts)  
**Risk Level:** HIGH for safety-critical system  
**Effort:** 3-4 days; Medium complexity

---

## Issue 1: Heavy Occlusion - Kalman Filter Timeout Too Aggressive

### Current State
**File:** `src/tracking/sort_tracker.hpp`
- `maxAge = 15` (default, OK for general tracking)
- But used as `max_age_` in `SortTracker` with effective timeout
- Stationary tracking: `MAX_STATIONARY_AGE_LOCAL` extends persistence
- Issue: When worker is briefly occluded, track gets dropped

**Current Code Flow:**
```cpp
if ((*it)->getTimeSinceUpdate() > max_age_) {
    int effective_max_age = max_age_;
    if ((*it)->isStationary()) {
        effective_max_age = std::max(max_age_, (*it)->getTimeSinceUpdate() + MAX_STATIONARY_AGE_LOCAL);
    }
    if ((*it)->getTimeSinceUpdate() > effective_max_age) {
        // Drop track
    }
}
```

### Root Causes
1. **Aggressive timeout:** max_age=15 frames at 25 FPS = 0.6 seconds max occlusion tolerance
2. **No historical context:** Occluded person resuming detection may get new ID instead of re-linking
3. **No Re-ID fallback:** Visual feature comparison has high threshold (0.5)

### Recommended Fixes

**1. Increase max_age** (Immediate, Low Risk)
```cpp
// From: maxAge = 15
// To:
maxAge = 45  // 1.8 seconds @ 25 FPS (industry standard for SORT)
```

**2. Implement Re-ID Re-linking** (Medium Effort)
- When a track is dropped, keep its visual features in a "ghost" list
- If new detection matches a ghost (cosine similarity > threshold), re-link instead of creating new ID
- Max 30 seconds of ghost persistence

**3. Make max_age Configurable** (Low Effort)
- Add to `AppConfig` struct: `int tracking_max_age = 45;`
- Add to `config.json`: `"tracking": { "max_age": 45 }`
- Pass to `SortTracker` constructor

**4. Add Occlusion Detection Heuristic** (Medium Effort)
- If bounding box area suddenly drops by >50% but object is confirmed, mark as "likely occluded"
- Don't count this frame against time-since-update
- Extend max_age for occluded objects to 60 frames (2.4 seconds)

### Expected Improvements
- Light occlusion (0.5-1.5 seconds): ✅ Fully resolved
- Heavy/extended occlusion (2+ seconds): ⚠️ Improved; may still drop if occluded longer than extension

### Impact on Other Components
- Minimal: Changes are in `sort_tracker.cpp` only
- Backward compatible: Default behavior same as current

### Files to Modify
- `src/tracking/sort_tracker.hpp` — Add `max_age_extended` member
- `src/tracking/sort_tracker.cpp` — Implement occlusion logic + re-linking
- `src/utils/config_loader.hpp` — Add tracking config
- `config.json` + `config.schema.json` — New parameters

---

## Issue 2: Extreme Lighting - No Preprocessing

### Current State
**File:** `src/inference/inference_engine.cpp` → `preprocess()` method
- Current preprocessing: Resize + normalize (min/max → [0, 1])
- No lighting correction
- YOLOv11-Nano trained on balanced lighting; fails on:
  - Glare/backlit → Feature washout
  - Darkness → Low SNR, false negatives

### Root Causes
1. **No adaptive histogram equalization:** Dynamic range not exploited
2. **No glare masking:** Bright areas wash out person silhouette
3. **No noise reduction:** Low light amplifies thermal noise

### Recommended Fix: CLAHE (Contrast-Limited Adaptive Histogram Equalization)

**Algorithm:**
```
Input: frame
1. Convert BGR → LAB color space (preserve color, adjust brightness)
2. Apply CLAHE to L (lightness) channel:
   - Tile size: 8×8 (tunable)
   - Contrast limit: 2.0 (tunable)
3. Convert LAB → BGR
4. Apply Gaussian blur (2-3 pixel radius) to reduce CLAHE artifacts
5. Continue existing pipeline (resize, normalize)
```

**Why CLAHE:**
- ✅ Local contrast enhancement (works per region)
- ✅ Removes glare by capping local max
- ✅ Boosts dark areas (amplify low signal)
- ✅ Fast: O(n) in image size (~2-3ms for 640×640)
- ✅ Parameter-tunable without retraining YOLO

**Implementation:**
```cpp
void InferenceEngine::preprocess(const cv::Mat& input, cv::Mat& output) {
    // 1. Convert to LAB
    cv::Mat lab;
    cv::cvtColor(input, lab, cv::COLOR_BGR2Lab);
    
    // Split channels
    std::vector<cv::Mat> lab_channels;
    cv::split(lab, lab_channels);
    
    // Apply CLAHE to L channel
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(2.0, cv::Size(8, 8));
    clahe->apply(lab_channels[0], lab_channels[0]);
    
    // Merge back
    cv::Mat lab_corrected;
    cv::merge(lab_channels, lab_corrected);
    
    // Convert back to BGR
    cv::Mat corrected;
    cv::cvtColor(lab_corrected, corrected, cv::COLOR_Lab2BGR);
    
    // Blur to reduce artifacts
    cv::GaussianBlur(corrected, corrected, cv::Size(3, 3), 1.0);
    
    // Existing resize + normalize logic
    cv::resize(corrected, output, cv::Size(config_.input_width, config_.input_height));
    output.convertTo(output, CV_32F);
    output = (output - 127.5f) / 127.5f;  // Normalize to [-1, 1]
}
```

**Parameters (Tunable):**
- `clipLimit = 2.0` — Contrast amplification (1.0-4.0 range typical)
- `tileGridSize = (8, 8)` — Larger = more global; smaller = more local
- `blurKernel = (3, 3)` — Reduce tile artifacts

### Configuration
Add to `config.json`:
```json
"inference": {
  "clahe_enabled": true,
  "clahe_clip_limit": 2.0,
  "clahe_tile_size": 8,
  "clahe_blur_kernel": 3
}
```

### Expected Improvements
- Glare: ✅ 80-90% improvement (recovered workers from washed-out regions)
- Darkness: ✅ 70-85% improvement (dark areas now visible to YOLO)
- Normal lighting: ✅ Neutral or slight improvement (no degradation)

### Performance Impact
- CPU: +2-3ms per frame (negligible at 25 FPS)
- GPU: None (preprocessing on CPU)
- Memory: Minimal (temporary buffers)

### Testing Strategy
1. **Benchmark:** Run YOLO on glare/dark footage with/without CLAHE
2. **Metrics:** Compare detection count, confidence, bounding box quality
3. **Edge cases:** Sunrise/sunset, artificial lights, IR illumination

### Files to Modify
- `src/inference/inference_engine.cpp` — Add CLAHE to `preprocess()`
- `src/inference/inference_engine.hpp` — Add CLAHE parameters
- `src/utils/config_loader.hpp` — Add inference config section
- `config.json` + `config.schema.json` — CLAHE parameters

---

## Issue 3: Perspective Errors - False Positives at Zone Boundaries  

### Current State
**File:** `src/pipeline/pipeline_manager.cpp` → `checkAlerts()`

**Current Logic:**
```cpp
// Person's feet position (bottom center of bounding box)
cv::Point2f bottom_center(bbox.x + bbox.width / 2, bbox.y + bbox.height);
bool in_zone = cv::pointPolygonTest(zone.polygon, bottom_center, false) > 0;
```

**Problem:**
- Bounding box edge detection is ±3-5 pixels error
- Worker leaning forward → Foot point drifts outside zone → False negative
- Worker tilted at angle → Foot point drifts → False positive at boundary
- No consideration of real-world 3D footprint

### Root Causes
1. **Single point test:** Only checks one pixel (bottom-center); no margin
2. **2D projection assumption:** Assumes camera is perfectly vertical
3. **No calibration:** Doesn't use perspective transform even if available
4. **Rigid boundary:** No soft threshold near edge

### Recommended Fixes

**1. Bounding Box Footprint Projection** (Medium Effort)
Instead of single point, project entire box bottom edge:
```cpp
// Project 4 corners of bbox bottom
cv::Rect2f bbox = detection.box;
std::vector<cv::Point2f> footprint = {
    {bbox.x, bbox.y + bbox.height},           // Bottom-left
    {bbox.x + bbox.width, bbox.y + bbox.height},  // Bottom-right
    {bbox.x + bbox.width/2, bbox.y + bbox.height}  // Bottom-center
};

// Test if ANY point is in zone (majority voting)
int points_in_zone = 0;
for (const auto& pt : footprint) {
    if (cv::pointPolygonTest(zone.polygon, pt, false) > 0) {
        points_in_zone++;
    }
}

// Require at least 2/3 of points in zone to trigger alert
bool is_violating = (points_in_zone >= 2);
```

**2. Calibration-Aware Perspective Transform** (Higher Effort)
If spatial mapper has calibration data, use `perspectiveTransform()`:
```cpp
// In SpatialMapper, if calibration exists:
cv::Point2f bottom_center_image = {bbox.x + bbox.width/2, bbox.y + bbox.height};
cv::Point2f bottom_center_world = spatial_mapper_->mapToWorld(bottom_center_image);

// Check world coordinates against zone
bool is_violating = isInZoneWorld(bottom_center_world, zone);
```

**3. Soft Boundary Threshold** (Low Effort)
Add small margin (±5 pixels) near boundary; only trigger on confident crossings:
```cpp
float BOUNDARY_MARGIN = 5.0f;
float margin_distance = cv::pointPolygonTest(zone.polygon, bottom_center, true);

bool is_violating = margin_distance > -BOUNDARY_MARGIN;  // Allow small overshoot
```

**4. Make Perspective Mode Configurable** (Low Effort)
```json
"zone_detection": {
  "mode": "projection",  // "point" | "footprint" | "calibrated"
  "boundary_margin": 5,
  "footprint_voting": "majority"  // require 2/3 of points in zone
}
```

### Configuration Examples

**Conservative (Reduce False Positives):**
```json
{
  "zone_detection": {
    "mode": "footprint",
    "boundary_margin": 10,
    "footprint_voting": "majority"
  }
}
```

**Aggressive (Reduce False Negatives):**
```json
{
  "zone_detection": {
    "mode": "point",
    "boundary_margin": -5
  }
}
```

### Expected Improvements
- False positives at boundary: ✅ 70% reduction
- False negatives (leaning): ✅ 50% reduction
- Precision (specificity): ✅ Improved
- Recall (sensitivity): ✅ Improved

### Files to Modify
- `src/pipeline/pipeline_manager.cpp` — Update `checkAlerts()` logic
- `src/spatial/spatial_mapper.hpp/cpp` — Add `getCalibrationStatus()` method
- `src/utils/config_loader.hpp` — Add zone_detection config
- `config.json` + `config.schema.json` — Zone detection parameters

---

## Issue 4: Configuration & Tuning Strategy

### Current Hardcoded Values (Scattered)

| Parameter | Current | File | Tunable? |
|-----------|---------|------|----------|
| `max_age` | 15 | sort_tracker.hpp | ❌ No |
| `conf_threshold` | 0.20 | inference_engine.hpp | ❌ No |
| `nms_threshold` | 0.50 | inference_engine.hpp | ❌ No |
| `CLAHE enabled` | N/A | Not implemented | ❌ No |
| Zone boundary margin | 0 | pipeline_manager.cpp | ❌ No |

### Solution: Centralized Configuration

**New `config.json` Section:**
```json
{
  "detection_tuning": {
    "confidence_threshold": 0.20,
    "nms_threshold": 0.50,
    "inference_interval": 1
  },
  "tracking_tuning": {
    "max_age": 45,
    "min_hits": 3,
    "iou_threshold": 0.3,
    "feature_threshold": 0.5,
    "occlusion_extension_frames": 15
  },
  "preprocessing": {
    "clahe_enabled": true,
    "clahe_clip_limit": 2.0,
    "clahe_tile_size": 8,
    "clahe_blur_kernel": 3
  },
  "zone_detection": {
    "mode": "footprint",
    "boundary_margin": 5,
    "footprint_voting": "majority"
  }
}
```

**Adding to AppConfig:**
```cpp
struct DetectionTuning {
    float confidence_threshold = 0.20f;
    float nms_threshold = 0.50f;
    int inference_interval = 1;
};

struct TrackingTuning {
    int max_age = 45;
    int min_hits = 3;
    float iou_threshold = 0.3f;
    float feature_threshold = 0.5f;
    int occlusion_extension_frames = 15;
};

struct PreprocessingConfig {
    bool clahe_enabled = true;
    float clahe_clip_limit = 2.0f;
    int clahe_tile_size = 8;
    int clahe_blur_kernel = 3;
};

struct AppConfig {
    // ... existing fields ...
    DetectionTuning detection;
    TrackingTuning tracking;
    PreprocessingConfig preprocessing;
};
```

### Files to Modify
- `src/utils/config_loader.hpp` — Add all tuning structs
- `src/utils/config_loader.cpp` — Parse all tuning sections
- `src/inference/inference_engine.hpp/cpp` — Use config for thresholds
- `src/tracking/sort_tracker.hpp/cpp` — Use config for max_age, etc.
- `config.json` + `config.schema.json` — Add sections + validation

---

## Implementation Priority & Timeline

| Priority | Issue | Effort | Time | Pre-req | Impact |
|----------|-------|--------|------|---------|--------|
| **1** | Config centralization (Issue 4) | Low | 2h | None | ✅ Enables all others |
| **2** | Occlusion + max_age (Issue 1) | Low | 2h | #1 | ⭐⭐⭐ HIGH |
| **3** | CLAHE preprocessing (Issue 2) | Medium | 3h | #1 | ⭐⭐⭐ HIGH |
| **4** | Perspective logic (Issue 3) | Medium | 3h | #1 | ⭐⭐ MEDIUM |

**Total Effort:** 10 hours (2-3 days with testing)

---

## Acceptance Criteria

### Occlusion Fix
- [ ] max_age configurable (default 45)
- [ ] Re-ID ghost list implements 30-second persistence
- [ ] Occlusion detection heuristic prevents time-since-update increment
- [ ] Light occlusions (0.5-1.5s) result in ID persistence >90%
- [ ] Config example in config.json shows recommended values

### CLAHE Preprocessing
- [ ] CLAHE configurable (enabled/disabled + parameters)
- [ ] Glare/dark footage shows >70% detection improvement
- [ ] Normal lighting shows ≥95% performance retention
- [ ] Processing time <3ms per frame on CPU
- [ ] Config example documents all CLAHE parameters

### Perspective Detection
- [ ] Footprint-based detection mode implemented and configurable
- [ ] Boundary margin parameter adjustable (±20 to +20 range)
- [ ] False positive rate at boundaries reduced by ≥50%
- [ ] False negative rate for leaning workers reduced by ≥30%
- [ ] Calibration-aware mode optional (uses spatial_mapper if available)

### Configuration Centralization
- [ ] All tunables in config.json (not hardcoded)
- [ ] config.schema.json validates all ranges and types
- [ ] config.json.example includes tuning recommendations
- [ ] Documentation explains each parameter's impact
- [ ] Tuning guide added to docs/ (e.g., TUNING.md)

---

## Risk Mitigation

### Potential Issues
1. **CLAHE artifacts** → Mitigate: Gaussian blur post-processing
2. **Footprint voting false negatives** → Mitigate: Configurable voting threshold
3. **Performance regression** → Mitigate: Benchmark on real hardware before merge
4. **Config parsing errors** → Mitigate: Schema validation + defaults

### Validation Strategy
- Unit tests for each tuning component (detection, tracking, preprocessing)
- Integration tests on existing video dataset
- Edge case testing: occlusion scenarios, lighting extremes
- Performance profiling before/after CLAHE

---

## Future Enhancements (Post-v1.0)

1. **Re-Identification (Re-ID) Models:** Replace color histogram with CNN embeddings
2. **3D Pose Estimation:** Use OpenPose to track feet vs. center of mass
3. **Multi-Camera Tracking:** Link tracks across cameras
4. **Adaptive Thresholds:** ML-based tuning recommendations
5. **Glare Detection:** Mask high-brightness regions before YOLO inference

