# Implementation Plan: Data Persistence & Intelligent Alerting

## Phase 1: SQLite Integration & Logging
- [x] **Dependencies:** Update `CMakeLists.txt` to include `SQLite3` (via FindPackage or FetchContent).
- [x] **Core Logic:** Create `src/utils/violation_logger.hpp` and `.cpp`.
  - [x] Implement `init(db_path)`: Open db and create table if not exists.
  - [x] Implement `log_violation(zone_id, confidence)`: Insert record.
- [x] **Tests:** Create `tests/unit/test_violation_logger.cpp` to verify schema creation and insertion.

## Phase 2: Alert Throttling Logic
- [x] **Core Logic:** Create `src/utils/alert_throttler.hpp` and `.cpp`.
  - [x] Implement `should_alert(zone_id)`: Returns true if cooldown has passed.
- [x] **Tests:** Create `tests/unit/test_alert_throttler.cpp` to verify timing logic.

## Phase 3: System Integration
- [x] **Configuration:** Update `config.json` with `db_path` and `alert_cooldown`.
- [x] **Loader:** Update `ConfigLoader` struct and parsing logic.
- [x] **Pipeline:** Update `PipelineManager` to:
  - [x] Initialize `ViolationLogger` and `AlertThrottler`.
  - [x] Use `AlertThrottler` before calling `mqtt_client->publish`.
  - [x] Call `ViolationLogger->log_violation` (always, or subject to logic).
- [x] **Verification:** Run full suite `unit_tests`.

## Phase 4: Final Polish
- [x] **Documentation:** Update `conductor/tech-stack.md` to include SQLite.
- [x] **Cleanup:** Ensure temporary database files are handled in tests.
