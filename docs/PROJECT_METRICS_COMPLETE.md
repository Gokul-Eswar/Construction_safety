# Construction Safety Sentinel - Complete Metrics and Parameter Documentation

Date generated: 2026-04-17

This document consolidates all project metrics into one report-ready reference, including:
- What each metric means
- How it is computed
- Where the data comes from in this project
- Current measured value from this workspace
- Target value and interpretation

Primary data sources used:
- [docs/metrics_db_snapshot.json](docs/metrics_db_snapshot.json)
- [docs/metrics_runtime_snapshot.json](docs/metrics_runtime_snapshot.json)
- [config.json](config.json)
- [config.schema.json](config.schema.json)
- [src/pipeline/pipeline_manager.cpp](src/pipeline/pipeline_manager.cpp)
- [src/utils/latency_logger.cpp](src/utils/latency_logger.cpp)

## 1) Data Validity Status

1. Persisted safety event data is available and valid.
2. Runtime telemetry data is currently unavailable in the last capture window.
3. Model training metrics (mAP, confusion matrix) are not emitted by this deployed runtime and must come from training artifacts.

Interpretation:
- Database-backed safety metrics below are accurate for this environment snapshot.
- Runtime FPS and latency metrics are structurally supported by code, but current capture returned no telemetry because broker/engine stream path was inactive at capture time.

## 2) Core YOLO Model Metrics

These are required in academic/project evaluation and should come from YOLO training outputs.

### Metric definitions and formulas

- mAP@50
  - Meaning: Mean average precision at IoU threshold 0.5
  - Formula: class AP values averaged across classes at IoU=0.5
  - Target: greater than 85%

- mAP@50-95
  - Meaning: Mean average precision averaged over IoU thresholds 0.50 to 0.95
  - Target: greater than 70%

- Precision
  - Formula: TP / (TP + FP)
  - Target: greater than 80%

- Recall
  - Formula: TP / (TP + FN)
  - Target: greater than 75%

- Confusion Matrix
  - Meaning: class-wise prediction correctness and confusion
  - Recommended classes for this project context: Person, Helmet, No-Helmet, Vest, No-Vest, Background

### Current status in this workspace

- mAP@50: Not available in runtime artifacts
- mAP@50-95: Not available in runtime artifacts
- Precision: Not available in runtime artifacts
- Recall: Not available in runtime artifacts
- Confusion Matrix: Not available in runtime artifacts

Reason:
- Runtime service focuses on inference, alerting, telemetry, and persistence.
- Training/evaluation artifacts are not currently present in the repository snapshot.

Required artifacts to finalize this section:
- training results csv
- confusion matrix image or matrix export
- class-wise precision and recall outputs

## 3) Real-Time System Performance Metrics

These metrics are produced by the runtime architecture and are emitted through telemetry topics.

Telemetry implementation references:
- Telemetry publish loop: [src/pipeline/pipeline_manager.cpp](src/pipeline/pipeline_manager.cpp)
- Latency percentile computation including p99: [src/utils/latency_logger.cpp](src/utils/latency_logger.cpp)
- Telemetry schema consumers: [web/backend/src/mqttService.js](web/backend/src/mqttService.js)

### 3.1 Inference speed and throughput

- Metric: Stream FPS
  - Meaning: frames processed per second per stream
  - Source field: telemetry streams.<stream_id>.fps
  - Target: greater than 15 FPS on edge device

Current value:
- Not measured in latest runtime snapshot (no telemetry events received)

### 3.2 Latency metrics

- Metric family: per-stage latency in milliseconds
  - Key fields: avg, min, max, p99
  - Source: telemetry latency.<stage>
  - Stage examples from pipeline timers: inference, tracking, render, processing, end-to-end
  - Target example for alert path: less than 2 seconds end-to-end alert latency

Current value:
- Not measured in latest runtime snapshot (no telemetry events received)

### 3.3 GPU runtime metrics

- GPU utilization percent
- GPU temperature in celsius
- GPU memory used MB and total MB
- Source: telemetry gpu object

Current value:
- Not measured in latest runtime snapshot (no telemetry events received)

### 3.4 Runtime topic-level throughput (latest snapshot)

From [docs/metrics_runtime_snapshot.json](docs/metrics_runtime_snapshot.json):
- telemetry message count: 0
- heartbeat message count: 0
- violations topic count: 0
- cloud sync topic count: 0
- parse errors: 0

Interpretation:
- Telemetry path is configured but inactive in capture window.

## 4) Safety Effectiveness Metrics

These metrics are derived from persisted violation records in SQLite.

Data source:
- [docs/metrics_db_snapshot.json](docs/metrics_db_snapshot.json)
- Database table and fields: [web/backend/db.js](web/backend/db.js)

### 4.1 Violation volume and confidence

Current measured values:
- total violations: 1
- first timestamp: 2026-01-29 15:18:32
- last timestamp: 2026-01-29 15:18:32
- average confidence: 0.95
- minimum confidence: 0.95
- maximum confidence: 0.95

### 4.2 Violation distribution

- by day (last 30):
  - 2026-01-29: 1

- by zone:
  - zone 1: 1 violation, average confidence 0.95

- by camera:
  - unknown: 1

### 4.3 Upload and sync state

- uploaded_count: 0
- pending_count: 1

Interpretation:
- Store-and-forward path has pending sync records in this snapshot.

### 4.4 False positive and false negative rates

Definitions:
- False Positive Rate = FP / (FP + TN)
- False Negative Rate = FN / (FN + TP)

Current status:
- Not directly measurable from raw runtime database alone without labeled ground truth set.

Recommended method:
- Build labeled validation clip set
- Compare detections and alerts against human-validated annotations

### 4.5 Temporal stability and alert throttling

Implementation evidence:
- Consecutive-violation filtering and cooldown logic: [src/utils/alert_throttler.cpp](src/utils/alert_throttler.cpp)

What to report after A/B run:
- false alerts without temporal filter
- false alerts with temporal filter
- reduction percent = (without - with) / without x 100

Current status:
- A/B numeric result not yet captured in this workspace snapshot.

## 5) Reliability and Operations Metrics

### 5.1 Uptime

Definition:
- uptime percent = healthy runtime duration / total observation duration x 100

Implementation hooks:
- health heartbeat file updates in pipeline loop: [src/pipeline/pipeline_manager.cpp](src/pipeline/pipeline_manager.cpp)
- container health checks: [docker-compose.yml](docker-compose.yml)

Current status:
- No continuous observation log in snapshot to compute percent uptime yet.

### 5.2 Network robustness and recovery

Available indicators in telemetry schema:
- reconnect_count
- error_count
- stale_timeout_count
- restart_timeout_count
- teardown_timeout_count
- admission/degradation fields

Current status:
- Not available in latest runtime snapshot due to no telemetry messages.

### 5.3 Crash recovery time

How to measure:
1. Record timestamp at induced stop/crash.
2. Record timestamp of first healthy heartbeat after restart.
3. Recovery time = heartbeat_recovery_ts - crash_ts.

Current status:
- Not measured in this snapshot.

## 6) Dataset and Training Documentation Status

### Available in this workspace

- Runtime model files exist:
  - [yolov8n.pt](yolov8n.pt)
  - [yolo11n.pt](yolo11n.pt)
  - [yolov8n.onnx](yolov8n.onnx)
  - [yolo11n.onnx](yolo11n.onnx)

- Runtime configured model path:
  - model_path: yolov8n.onnx in [config.json](config.json)

### Missing for full model-evaluation section

- dataset split metadata
- class count and per-class instance counts
- training hyperparameter logs (epochs, batch size, image size)
- training and validation loss curves
- confusion matrix export
- mAP precision recall training outputs

## 7) Complete Runtime Parameter Documentation

This section documents each runtime parameter with role, allowed range, and current configured value.

### 7.1 Detection tuning

1. confidence_threshold
- Purpose: minimum confidence required to keep a detection
- Range: 0.05 to 0.95
- Current value: 0.20
- Effect: lower value increases recall but can increase false positives

2. nms_threshold
- Purpose: overlap threshold for non-maximum suppression
- Range: 0.10 to 0.95
- Current value: 0.50
- Effect: lower value suppresses more overlapping boxes

3. inference_interval
- Purpose: process every Nth frame
- Range: 1 to 30
- Current value: 1
- Effect: higher value lowers compute load but may miss fast events

### 7.2 Tracking tuning

1. max_age
- Purpose: max frames to keep a track alive without detection
- Range: 15 to 120
- Current value: 45

2. min_hits
- Purpose: detections required before a track is considered confirmed
- Range: 1 to 10
- Current value: 3

3. iou_threshold
- Purpose: threshold for matching detections to tracks by overlap
- Range: 0.10 to 0.90
- Current value: 0.30

4. feature_threshold
- Purpose: minimum appearance similarity for re-identification support
- Range: 0.30 to 0.90
- Current value: 0.50

5. occlusion_extension_frames
- Purpose: additional frames allowed during occlusion conditions
- Range: 5 to 60
- Current value: 15

### 7.3 Preprocessing

1. clahe_enabled
- Purpose: enable adaptive contrast correction
- Type: boolean
- Current value: true

2. clahe_clip_limit
- Purpose: cap local contrast amplification
- Range: 1.0 to 4.0
- Current value: 2.0

3. clahe_tile_size
- Purpose: local grid size for CLAHE
- Range: 4 to 16
- Current value: 8

4. clahe_blur_kernel
- Purpose: blur kernel to reduce local contrast artifacts
- Range: 1 to 7
- Current value: 3

### 7.4 Zone detection

1. mode
- Purpose: zone entry decision strategy
- Allowed values: point, footprint, calibrated
- Current value: footprint

2. boundary_margin
- Purpose: tolerance in pixels around zone boundaries
- Range: -20 to 20
- Current value: 5

3. footprint_voting
- Purpose: voting rule for multi-point footprint decision
- Allowed values: all, majority, any
- Current value: majority

### 7.5 Alerting and retention

1. alert_cooldown
- Purpose: minimum milliseconds between alerts for same zone/person pair
- Range: 1000 to 300000
- Current value: 5000

2. log_retention_days
- Purpose: retention period for violation logs
- Range: 1 to 365
- Current value: 30

### 7.6 MQTT and stream

1. mqtt.host
- Purpose: MQTT broker host
- Current value: localhost

2. mqtt.port
- Purpose: broker port
- Range: 1 to 65535
- Current value: 1883

3. mqtt.topic
- Purpose: base topic for violations
- Current value: safety/v1/violations

4. mqtt.client_id
- Purpose: unique client identifier for broker
- Current value: safety_engine_01

5. stream_port
- Purpose: MJPEG streaming port
- Range: 1024 to 65535
- Current value: 8081

## 8) Measured Metrics Table (Current Snapshot)

Metric | Current value | Target | Status | Source

Violations total | 1 | project dependent | Measured | metrics_db_snapshot.json
Violation avg confidence | 0.95 | high confidence desirable | Measured | metrics_db_snapshot.json
Uploaded violations | 0 | maximize | Measured | metrics_db_snapshot.json
Pending violations | 1 | minimize | Measured | metrics_db_snapshot.json
Runtime telemetry count | 0 | greater than 0 during active run | Not measured in active path | metrics_runtime_snapshot.json
Runtime heartbeat count | 0 | greater than 0 during active run | Not measured in active path | metrics_runtime_snapshot.json
FPS per stream | unavailable | greater than 15 FPS | Pending runtime capture | telemetry topics
Latency p99 | unavailable | less than target threshold | Pending runtime capture | telemetry topics
Uptime percent | unavailable | 99.5% | Pending observation window | health logs
mAP@50 | unavailable | greater than 85% | Pending training artifacts | training outputs
mAP@50-95 | unavailable | greater than 70% | Pending training artifacts | training outputs
Precision | unavailable | greater than 80% | Pending training artifacts | training outputs
Recall | unavailable | greater than 75% | Pending training artifacts | training outputs

## 9) Immediate Capture Procedure (To Fill Remaining Metrics)

Use this exact sequence before final presentation:

1. Start Docker Desktop.
2. Start full system via [tools/Sentinel.bat](tools/Sentinel.bat) option 1.
3. Ensure at least one stream is active.
4. Run:
   powershell -ExecutionPolicy Bypass -File .\\tools\\capture_presentation_metrics.ps1 -DurationSec 60

Generated outputs to use in report:
- [docs/metrics_db_snapshot.json](docs/metrics_db_snapshot.json)
- [docs/metrics_runtime_snapshot.json](docs/metrics_runtime_snapshot.json)

If runtime telemetry remains zero:
- verify MQTT broker availability on localhost:1883
- verify engine is connected and publishing to safety/v1 topics
- rerun capture for 60 seconds

## 10) Final Reporting Guidance

For academic integrity and presentation confidence:
- Mark each metric as Measured, Pending runtime capture, or Pending training artifact.
- Do not fabricate unavailable values.
- Use this document as the single source of truth and update only from generated snapshots.
