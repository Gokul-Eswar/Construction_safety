# Specification: Web Dashboard & Visualization

## 1. Overview
A web-based user interface to monitor construction safety status in real-time. It provides operators with a live view of active alerts and a searchable history of past violations.

## 2. Architecture
- **Monorepo Structure:** New `web/` directory containing:
  - `backend/`: Node.js Express server.
  - `frontend/`: React (Vite) application.

## 3. Backend (Node.js)
- **Database Access:** Read-only access to `safety_violations.db` (SQLite).
- **API Endpoints:**
  - `GET /api/violations`: Returns paginated list of violations.
  - `GET /api/stats`: Returns summary stats (e.g., total today).
- **MQTT Bridge:** (Optional) If browser cannot connect directly to broker, backend can proxy, but standard WebSockets over MQTT (port 9001) is preferred if Mosquitto is configured. For now, we assume direct frontend connection or simple polling if MQTT is complex to set up for the user. **Decision:** Use simple polling for DB updates + direct MQTT over WebSockets if available.

## 4. Frontend (React)
- **Framework:** React + Vite + TypeScript.
- **UI Library:** Material UI (MUI).
- **Pages:**
  - **Dashboard (Home):**
    - Status Indicators (System Online/Offline).
    - Recent Alerts List.
    - Stats Cards (Total Violations, Active Zones).
  - **History:**
    - Data Grid showing all past violations with filtering (Date, Zone).

## 5. Integration
- The C++ application writes to `safety_violations.db`.
- The Node.js backend reads from `safety_violations.db`.
- Shared `config.json` might be read by backend to know Zone names.

## 6. Constraints
- Must run locally.
- Minimal setup required.
