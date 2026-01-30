# Implementation Plan: Web Dashboard & Visualization

## Phase 1: Backend Setup
- [x] **Scaffold:** Create `web/backend` directory and `package.json`.
- [x] **Dependencies:** Install `express`, `sqlite3`, `cors`, `dotenv`.
- [x] **Database:** Implement `db.js` to connect to `../../safety_violations.db`.
- [x] **API:** Implement `server.js` with endpoints:
  - `/api/violations` (with basic pagination/limit).
  - `/api/config` (reads project `config.json` to serve zone info).
- [x] **Test:** Verify API returns data from the actual DB populated by the C++ app.

## Phase 2: Frontend Setup
- [x] **Scaffold:** Create `web/frontend` using Vite (React + TS).
- [x] **Dependencies:** Install `axis`, `@mui/material`, `@emotion/react`, `@emotion/styled`.
- [x] **Components:**
  - `Navbar`: Navigation bar.
  - `StatCard`: Simple display widget.
  - `ViolationTable`: Data grid.
- [x] **API Client:** Service to fetch data from backend.

## Phase 3: Integration & Real-time
- [x] **Polling:** Implement auto-refresh (e.g., every 5s) for the Dashboard view to see new violations.
- [x] **Config:** Display Zone names instead of IDs by matching with `config.json` data.

## Phase 4: Polish
- [x] **Styling:** Dark mode theme (Construction safety colors: Yellow/Black).
- [x] **Documentation:** Add `web/README.md` with start instructions.
