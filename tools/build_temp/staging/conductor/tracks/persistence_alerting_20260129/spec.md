# Specification: Data Persistence & Intelligent Alerting

## 1. Overview
This track focuses on transforming the system from a transient alert generator to a persistent safety auditing tool. It introduces a local database to store violation history and implements "intelligent throttling" to improve operator experience by reducing alert fatigue.

## 2. Requirements

### 2.1. Violation Persistence
- **Database:** SQLite (embedded, zero-configuration).
- **Schema:**
  - `violations` table:
    - `id`: Auto-increment integer.
    - `timestamp`: UTC timestamp (ISO 8601).
    - `object_id`: ID of the detected person (if tracking enabled) or generic identifier.
    - `zone_id`: ID of the safety zone.
    - `confidence`: Detection confidence score.
    - `snapshot_path`: Relative path to a saved image of the violation (future proofing, implementation optional in this track).
    - `is_active`: Boolean, indicating if the violation is currently ongoing (for duration tracking).

### 2.2. Alert Throttling
- **Goal:** Prevent flooding the MQTT topic with hundreds of messages for the same person standing in a zone.
- **Mechanism:** "Sliding Window Suppression".
- **Logic:**
  - If an alert for `(zone_id, object_id)` was sent within the last `T` seconds (configurable), do not send another MQTT alert.
  - Still log the violation to the database (or log it as a "continuation").
- **Configuration:**
  - `alert_throttle_interval`: Integer (seconds), default `5`.

### 2.3. Configuration Updates
- Add `database_path` to `config.json`.
- Add `alert_cooldown` to `config.json`.

## 3. Architecture Impact
- **New Component:** `ViolationLogger` (handles SQLite interactions).
- **New Component:** `AlertThrottler` (stateful tracker of recent alerts).
- **Modified Component:** `PipelineManager` (integrates the new components).
- **Modified Component:** `ConfigLoader` (parses new fields).

## 4. Performance Constraints
- Database writes must not block the main inference loop.
- Throttling checks must be O(1) or very fast.
