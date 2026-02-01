# Implementation Plan: Modern Web UI & System Control

## Phase 1: Backend API Extensions
- [x] **Task:** Implement `POST /api/config/streams` and `POST /api/config/global` in `server.js`.
    -   *Why:* To allow the UI to modify the configuration.
    -   *Files:* `web/backend/server.js`
- [x] **Task:** Implement `POST /api/system/restart` (Mocked or actual process restart).
    -   *Why:* To apply changes that require a restart.
    -   *Files:* `web/backend/server.js`

## Phase 2: UI Modernization (Foundation)
- [ ] **Task:** Setup MUI Dark Theme and Layout Component (Sidebar + Topbar).
    -   *Why:* To establish the modern visual direction.
    -   *Files:* `web/frontend/src/theme.ts`, `web/frontend/src/components/Layout.tsx`, `web/frontend/src/App.tsx`
- [ ] **Task:** Create `Dashboard` page with Tiled Feed and Status Widgets.
    -   *Why:* To provide the main monitoring view.
    -   *Files:* `web/frontend/src/pages/Dashboard.tsx`

## Phase 3: System Control Features
- [ ] **Task:** Create `StreamManager` page (List + Add/Edit Dialog).
    -   *Why:* To manage multiple cameras.
    -   *Files:* `web/frontend/src/pages/StreamManager.tsx`
- [ ] **Task:** Create `Settings` page (MQTT, Global Params).
    -   *Why:* To control system behavior.
    -   *Files:* `web/frontend/src/pages/Settings.tsx`
- [ ] **Task:** Integrate `ZoneEditor` into the new layout.
    -   *Why:* To maintain existing functionality.
    -   *Files:* `web/frontend/src/pages/ZoneEditorPage.tsx`

## Phase 4: Verification
- [ ] **Task:** Manual verification of all pages.
- [ ] **Task:** Verify config updates are saved to `config.json`.
