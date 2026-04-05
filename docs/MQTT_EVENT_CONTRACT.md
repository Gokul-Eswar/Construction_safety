# MQTT Event Contract - Construction Safety Sentinel

**Version:** 1.0  
**Date:** April 5, 2026  
**Status:** Active  

This document defines the MQTT event contract between the C++ Engine and Web Backend. All components **must** adhere to this specification.

---

## Overview

All MQTT topics use the versioned format: `safety/v1/<event_type>`

| Topic | Direction | Source | Subscriber | Purpose |
|-------|-----------|--------|-----------|---------|
| `safety/v1/violations` | C++ → Web | Engine | Backend | Person in restricted zone |
| `safety/v1/heartbeat` | C++ → Web | Engine | Backend | System health status |
| `safety/v1/telemetry` | C++ → Web | Engine | Backend | Performance metrics |
| `safety/v1/control` | Web → C++ | Backend | Engine | Commands (restart, config) |

---

## Event Definitions

### 1. Violation Alert: `safety/v1/violations`

**Direction:** C++ Engine → Web Backend  
**Frequency:** On zone violation (throttled by `alert_cooldown`)  
**QoS:** 1 (At least once)  

**Payload Schema:**
```json
{
  "version": "1.0",
  "timestamp": "2026-04-05T14:23:45.123Z",
  "event_id": "uuid-v4-string",
  "stream_id": "sim_01",
  "stream_name": "Simulation Feed - Zone A",
  "zone_id": 1,
  "zone_name": "High Hazard Area",
  "person_id": 42,
  "confidence": 0.95,
  "bounding_box": {
    "x": 150,
    "y": 120,
    "width": 80,
    "height": 140
  },
  "alert_level": "warning",
  "duration_seconds": 5.2
}
```

**Field Descriptions:**
- `version` (string): Payload version (always "1.0", used for compatibility checks)
- `timestamp` (ISO 8601): UTC time when violation detected
- `event_id` (UUID v4): Unique identifier for this event (for deduplication)
- `stream_id` (string): Stream identifier from config
- `stream_name` (string): Human-readable stream name
- `zone_id` (integer): Zone identifier
- `zone_name` (string): Human-readable zone name
- `person_id` (integer): Tracking ID of person in violation
- `confidence` (float): Detection confidence (0.0–1.0)
- `bounding_box` (object): Person's bounding box in pixels
  - `x`, `y`: Top-left corner
  - `width`, `height`: Box dimensions
- `alert_level` (enum): `warning` | `critical` (may indicate multiple violations)
- `duration_seconds` (float): How long person has been in zone

**Example (JavaScript):**
```javascript
const violation = {
  version: "1.0",
  timestamp: new Date().toISOString(),
  event_id: uuidv4(),
  stream_id: "sim_01",
  zone_id: 1,
  person_id: 42,
  confidence: 0.95,
  alert_level: "warning"
};
mqtt.publish('safety/v1/violations', JSON.stringify(violation));
```

---

### 2. Heartbeat: `safety/v1/heartbeat`

**Direction:** C++ Engine → Web Backend  
**Frequency:** Every 10 seconds (or on state change)  
**QoS:** 1 (At least once)  

**Payload Schema:**
```json
{
  "version": "1.0",
  "timestamp": "2026-04-05T14:23:45.123Z",
  "status": "running",
  "uptime_seconds": 3600,
  "streams_active": 1,
  "streams_total": 1,
  "gpu_available": true,
  "gpu_utilization_percent": 65.5,
  "detections_total": 1024,
  "violations_total": 12,
  "frame_rate": 25.5
}
```

**Field Descriptions:**
- `status` (enum): `running` | `degraded` | `error`
  - `running`: All systems operational
  - `degraded`: Some streams offline, partial detection
  - `error`: Critical failure (check logs)
- `uptime_seconds` (integer): Seconds since engine start
- `streams_active` (integer): Number of streams currently processing
- `streams_total` (integer): Configured stream count
- `gpu_available` (boolean): GPU is available/working
- `gpu_utilization_percent` (float): GPU usage (0–100)
- `detections_total` (integer): Cumulative detections since start
- `violations_total` (integer): Cumulative violations logged
- `frame_rate` (float): Current avg FPS across streams

**Health Interpretation:**
| Status | Action | Alert |
|--------|--------|-------|
| running | Dashboard green | None |
| degraded | Yellow indicator, log lines reviewed | Check stream connectivity |
| error | Red indicator, high priority | Restart recommended |

---

### 3. Telemetry: `safety/v1/telemetry`

**Direction:** C++ Engine → Web Backend  
**Frequency:** Every 30 seconds  
**QoS:** 1  

**Payload Schema:**
```json
{
  "version": "1.0",
  "timestamp": "2026-04-05T14:23:45.123Z",
  "latency_ms": {
    "mean": 45.2,
    "p50": 42,
    "p95": 58,
    "p99": 72
  },
  "memory_usage_mb": 512,
  "memory_limit_mb": 2048,
  "cpu_percent": 22.5,
  "inference_time_ms": 35.0,
  "streams": [
    {
      "id": "sim_01",
      "frames_processed": 15234,
      "fps": 25.5,
      "latency_ms": 45
    }
  ]
}
```

**Field Descriptions:**
- `latency_ms` (object): End-to-end inference latency statistics
  - `mean`: Average latency
  - `p50`, `p95`, `p99`: Percentile latencies
- `memory_usage_mb` (integer): Current process memory
- `memory_limit_mb` (integer): Memory ceiling (for monitoring)
- `cpu_percent` (float): CPU usage (0–100)
- `inference_time_ms` (float): Average model execution time
- `streams` (array): Per-stream statistics

**Usage:**
- Web dashboard plots latency trends
- Alerts if p99 > threshold (e.g., 200ms)
- Memory monitoring for OOM prevention

---

### 4. Control Command: `safety/v1/control`

**Direction:** Web Backend → C++ Engine  
**Frequency:** On demand  
**QoS:** 1  

**Payload Schema:**
```json
{
  "version": "1.0",
  "command": "restart",
  "initiator": "web_ui",
  "reason": "user_requested",
  "config_reload": false
}
```

**Command Types:**

#### 4a. Restart Command
```json
{
  "version": "1.0",
  "command": "restart",
  "initiator": "web_ui|api|scheduler",
  "reason": "user_requested|health_check|memory_warning|gpu_error"
}
```
- Engine logs, stops streams, exits cleanly
- Launcher (Sentinel.bat/sh) restarts automatically
- No loss of database records

#### 4b. Reload Config
```json
{
  "version": "1.0",
  "command": "reload_config",
  "initiator": "web_ui",
  "reason": "settings_changed",
  "config_reload": true
}
```
- Engine reloads config.json without stopping
- Applies new alert_cooldown, inference_interval, etc.
- Currently active streams continue (may restart on config change)

#### 4c. Shutdown Command
```json
{
  "version": "1.0",
  "command": "shutdown",
  "initiator": "api|scheduler",
  "reason": "maintenance|deployment"
}
```
- Graceful shutdown (opposite of restart)
- Engine terminates cleanly

**Response:** No explicit response on topic; instead, Engine publishes updated `heartbeat` with new `status`.

---

## Version Negotiation

### On Connection

**C++ Engine publishes (immediate):**
```json
{
  "version": "1.0",
  "supported_versions": ["1.0"],
  "engine_id": "engine_01",
  "timestamp": "2026-04-05T14:23:45.123Z"
}
```
Topic: `safety/v1/handshake`

**Web Backend subscribes to `safety/v1/handshake` and:**
1. Logs supported version
2. If no match → Log error and fall back to v1.0
3. If match → Begin publishing commands

### Version Mismatch Handling

| Scenario | Behavior |
|----------|----------|
| Engine v1.0, Web v1.0 | ✅ OK, use v1.0 |
| Engine v1.0, Web v2.0 (future) | ⚠️ Web falls back to v1.0 payloads |
| Engine v2.0, Web v1.0 | ⚠️ Engine falls back to v1.0 or logs error |

---

## Field Validation Rules

### Zone ID Compatibility
- Accept both integer (preferred) and numeric string
- Normalize to integer internally
- Log warning if string used (migrate to integer recommended)

### UUID Format
- Use UUID v4 for event_id
- Format: `550e8400-e29b-41d4-a716-446655440000`
- Enables deduplication across network restarts

### Confidence Range
- Always 0.0 to 1.0
- Reject if outside range (don't publish)
- Log rejected detections

### ISO 8601 Timestamps
- Always UTC (Z suffix)
- Millisecond precision minimum
- Format: `2026-04-05T14:23:45.123Z`

---

## Error Handling

### Publishing Failures
- Engine: Log error, retry next interval
- Web: Log error, continue processing other topics

### Invalid Payloads
- Receiver: Log error with topic and payload snippet
- Receiver: Continue (don't crash)
- Receiver: Increment error counter
- Receiver: Alert if error rate > 10 errors/minute

### Connection Loss
- Engine: Reconnect with exponential backoff (max 30s)
- Web: Reconnect with exponential backoff (max 30s)
- Both: Publish housekeeping events (heartbeat) on reconnect

---

## Quality of Service (QoS)

| Topic | QoS | Reason |
|-------|-----|--------|
| violations | 1 | Human-safety-critical; must arrive ≥1x |
| heartbeat | 1 | Health-critical; must verify reception |
| telemetry | 0 | Informational; ok to lose isolated samples |
| control | 1 | Commands must be received |
| handshake | 1 | Version critical; must verify |

---

## Rate Limiting

| Topic | Min Interval | Max Interval | Burst |
|-------|---|---|---|
| violations | alert_cooldown (5s default) | None (per violation) | 1 per zone+person |
| heartbeat | 10s | 10s | Steady |
| telemetry | 30s | 30s | Steady |
| control | N/A | N/A | On-demand |

---

## Backward Compatibility

### v1.0 → v1.1 Future Migration Path
1. Add new optional fields to payloads
2. Old receivers ignore unknown fields (JSON spec)
3. New receivers set defaults for missing fields
4. Increment payload version to 1.1 when ready

### Breaking Changes (v1.x → v2.0)
1. Announce 3 months in advance
2. Support both v1.0 and v2.0 simultaneously (dual publish)
3. Migrate all instances
4. Retire v1.0

---

## Implementation Checklist

### C++ Engine (src/utils/mqtt_client.cpp, src/pipeline/pipeline_manager.cpp)
- [ ] Publish to `safety/v1/*` topics (not `safety/*`)
- [ ] Include `version: "1.0"` in all payloads
- [ ] Generate UUID v4 for event_id
- [ ] Format timestamps as ISO 8601 UTC
- [ ] Validate zone_id is numeric
- [ ] Publish `safety/v1/handshake` on connect
- [ ] Handle restart/reload commands on `safety/v1/control`
- [ ] Log schema violations with remedy suggestions

### Web Backend (web/backend/src/mqttService.js, routes.js)
- [ ] Subscribe to `safety/v1/*` topics
- [ ] Validate incoming payloads against schema
- [ ] Log validation errors
- [ ] Download current `config.schema.json` on startup
- [ ] Publish versioned control commands
- [ ] Emit Socket.IO events with validated payloads to frontend
- [ ] Track version negotiation status in logs

### Frontend (web/frontend/src/)
- [ ] Display violation alerts from versioned payloads
- [ ] Plot telemetry trends
- [ ] Show health status from heartbeat
- [ ] Provide UI for control commands (restart, reload config)

---

## Monitoring & Observability

### Metrics to Track
- Violations/hour (by stream, by zone)
- Latency p99 (alert if > 200ms)
- Heartbeat reception rate (alert if < 95%)
- Command latency (time to acknowledge restart)

### Logging Standards
- All MQTT operations log topic, version, timestamp
- Invalid payloads logged with full detail (for debugging)
- Version mismatches logged as warnings (not errors if fallback available)

### Dashboard Indicators
- Green: heartbeat status = running, fps > 15, latency p99 < 100ms
- Yellow: heartbeat status = degraded OR latency p99 > 100ms
- Red: heartbeat status = error OR no heartbeat in 30s

