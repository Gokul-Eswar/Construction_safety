# Architecture & Code Quality Fixes - Implementation Plan

Date: April 5, 2026  
Version: 1.0 (Planning Phase)

---

## Issue 1: main.cpp Control-Flow Conflicts

### Current State
The current `src/main.cpp` has a relatively clean single lifecycle:
1. Parse arguments (`--config`, `--build-engine-only`, positional config)
2. Load config via ConfigLoader
3. If `--build-engine-only`: Load model and exit
4. Otherwise: Full system initialization
5. Main loop with signal handling
6. Graceful shutdown

**Finding:** The code has already been refined to a single path. However, there's room for improvement:
- No explicit state machine or lifecycle verification
- No health check mechanism during main loop
- No structured logging of state transitions

### Recommended Fixes

**1. Add Explicit Lifecycle State Machine**
- Create `SystemState` enum: `NONE → INITIALIZING → RUNNING → SHUTTING_DOWN → STOPPED`
- Add state validation at each transition
- Log all state changes

**2. Add Health Check Loop**
- Monitor pipeline status during main loop
- Detect stale streams or deadlocks
- Implement periodic heartbeat publication

**3. Refactor for Testability**
- Extract lifecycle logic into `SystemLifecycle` class
- Make main.cpp a thin orchestrator
- Enable unit testing of lifecycle transitions

### Files to Modify
- `src/main.cpp` — Refactor to SystemLifecycle wrapper
- **NEW:** `src/system_lifecycle.hpp` — State machine and health monitoring
- **NEW:** `src/system_lifecycle.cpp` — Implementation

---

## Issue 2: Configuration Schema Drift

### Current State
**config.json** (actual):
```json
{
    "streams": [],           // Empty!
    "model_path": "...",
    "mqtt": { "host": "..." }
}
```

**config.json.example** (reference):
```json
{
    "streams": [
        {
            "id": "cam_01",
            "name": "Front Gate",
            "type": "rtsp",
            "uri": "rtsp://...",
            "zones": [
                { "id": 1, "name": "...", "points": [...] }
            ]
        }
    ],
    "model_path": "...",
    "mqtt": { ... }
}
```

**Problem:**
- Active config file is empty
- Example file has full schema but not used by default
- Web backend can't properly validate against schema
- Users don't know expected format without reading example file

### Recommended Fixes

**1. Populate config.json with Example Data**
- Copy full structure from config.json.example to config.json
- Use "Simulation Feed" settings for demo mode (no camera required)
- Document each field with inline comments

**2. Create JSON Schema File**
- **NEW:** `config.schema.json` — Formal JSON Schema 5
- Specifies required fields, types, constraints
- Enables validation in web backend and C++

**3. Add Config Validation**
- Update `ConfigLoader::load()` to validate against schema
- Fail fast with clear error messages for missing/invalid fields
- Add schema version field for future migrations

**4. Update Documentation Link**
- Update README and launcher to reference both example and schema
- Clarify: "See config.json.example for demo mode defaults"

### Files to Modify
- `config.json` — Populate with full working example (simulation feed)
- **NEW:** `config.schema.json` — Formal JSON Schema
- `src/utils/config_loader.cpp` — Add schema validation
- `web/backend/src/configManager.js` — Add schema validation
- `readme.md` — Link to schema documentation

---

## Issue 3: MQTT Event Contract Issues

### Current State

**Topics Used (ad-hoc, no versioning):**
```
safety/alerts         → Violation alerts (payload: undefined)
safety/heartbeat      → System heartbeat (payload: undefined)
safety/telemetry      → System metrics (payload: undefined)
safety/cloud_sync     → Cloud sync events (payload: undefined)
safety/control        → Control commands (e.g., restart)
```

**Problems:**
- No versioning (breaking changes will cause incompatibility)
- Payload schemas undefined (arbitrary JSON)
- No documentation of required/optional fields
- No version negotiation between C++ and web
- New features require ad-hoc topic additions

### Recommended Fixes

**1. Create MQTT Event Contract Schema**
- **NEW:** `docs/MQTT_EVENT_CONTRACT.md` — Versioned specification
- Define all topics with versions (e.g., `safety/v1/alerts`)
- Specify required/optional payload fields
- Document backward compatibility strategy

**2. Publish Topics with Version Prefix**
- Change topics to: `safety/v1/alerts`, `safety/v1/heartbeat`, etc.
- Implement version negotiation on connect
- Allow deprecation warnings in logs

**3. Define Payload Schemas**

Example format:
```typescript
// safety/v1/violation (C++ → Web)
{
  "version": "1.0",
  "timestamp": "2026-04-05T12:00:00Z",
  "event_id": "uuid",
  "stream_id": "cam_01",
  "zone_id": 1,
  "person_id": 42,
  "confidence": 0.95,
  "boundingbox": [x, y, w, h],
  "alert_level": "warning|critical"
}

// safety/v1/heartbeat (C++ → Web)
{
  "version": "1.0",
  "timestamp": "...",
  "status": "running|degraded|error",
  "uptime_seconds": 3600,
  "streams_active": 2,
  "gpu_utilization": 0.65
}

// safety/v1/telemetry (C++ → Web)
{
  "version": "1.0",
  "timestamp": "...",
  "fps": [25.5, 24.8, 25.2],
  "latency_ms": [45, 52, 48],
  "detections_total": 1024,
  "violations_total": 12
}

// safety/v1/control (Web → C++)
{
  "version": "1.0",
  "command": "restart|reload_config|shutdown",
  "initiator": "web_ui|api|scheduler",
  "reason": "user_request|config_change|health_check"
}
```

**4. Update C++ Engine**
- Create `EventSchema` class for each topic
- Serialize/deserialize using structured approach
- Add version validation
- Log schema mismatches

**5. Update Web Backend**
- Import event schemas
- Validate incoming messages
- Emit versioned events to Socket.IO
- Log contract violations

**6. Add Version Negotiation**
- On MQTT connect, C++ publishes supported versions
- Web backend sends acknowledgment
- Both sides use highest common version

### Files to Modify
- **NEW:** `docs/MQTT_EVENT_CONTRACT.md` — Contract specification
- **NEW:** `src/utils/event_schema.hpp` — Event payload definitions
- **NEW:** `src/utils/event_schema.cpp` — Serialization/validation
- `src/utils/mqtt_client.cpp` — Version handling
- `src/pipeline/pipeline_manager.cpp` — Use versioned topics
- `web/backend/src/mqttService.js` — Version handling and validation
- `web/backend/src/events.js` — Event payload definitions (NEW)

---

## Implementation Priority & Timeline

| Priority | Issue | Effort | Time | Impact |
|----------|-------|--------|------|--------|
| **1** | Config Schema (Issue 2) | Low | 1-2h | HIGH - Blocks users from running demo |
| **2** | MQTT Event Contract (Issue 3) | Medium | 2-3h | MEDIUM - Enables future integrations |
| **3** | Lifecycle Improvements (Issue 1) | Medium | 2-3h | MEDIUM - Improves reliability |

---

## Acceptance Criteria

### Config Schema Fix
- [ ] `config.json` populated with working demo configuration
- [ ] `config.schema.json` created and validates both files
- [ ] ConfigLoader validates config against schema on load
- [ ] Error messages clearly indicate schema violations
- [ ] Web backend can be tested with schema validation

### MQTT Event Contract Fix
- [ ] All topics use `safety/v1/<event>` format
- [ ] `MQTT_EVENT_CONTRACT.md` documents all topics and payloads
- [ ] C++ engine validates and publishes versioned events
- [ ] Web backend validates all incoming MQTT messages
- [ ] Version mismatch logged with clear warnings
- [ ] No breaking changes without major version bump

### Lifecycle Improvements
- [ ] Multiple state transitions logged and traceable
- [ ] Health check runs every 10 seconds
- [ ] Deadlock detected within 30 seconds
- [ ] Graceful shutdown completes within 5 seconds
- [ ] Unit tests verify state machine transitions

