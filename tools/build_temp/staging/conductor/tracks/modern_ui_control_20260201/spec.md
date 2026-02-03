# Specification: Modern Web UI & System Control

## Context
The current UI is functional but basic. The user requires a "modern" look and "full control" over the system. This implies not just viewing data, but managing the system configuration and lifecycle.

## Goals
1.  **Visual Overhaul:** Implement a modern, dark-themed Dashboard with a responsive sidebar layout.
2.  **Stream Management:** dedicated page to manage RTSP streams (Add/Edit/Delete).
3.  **System Control:** Capability to restart the service or reload configuration via the UI.
4.  **Global Settings:** Form-based editing of global settings (MQTT, Alert Cooldown, etc.).

## Requirements
-   **Framework:** React + Material UI (MUI).
-   **Theme:** Dark Mode by default with custom palette (Industrial Safety colors: Yellow/Black/Dark Grey).
-   **Navigation:** Persistent Sidebar.
-   **Pages:**
    -   **Dashboard:** Live 2x2 Feed, Critical Alerts, System Health.
    -   **Streams:** List of cameras with status and edit actions.
    -   **Zones:** Enhanced Zone Editor.
    -   **Logs:** Searchable violation history.
    -   **Settings:** Global system configuration.

## Backend Changes
-   **API:**
    -   `POST /api/restart`: Endpoint to trigger service restart (or just reload config).
    -   `POST /api/streams`: CRUD for streams.
    -   `POST /api/settings`: Update global config.

## Verification
-   Visual inspection of the new Dark Mode.
-   Successful addition of a new mock stream via UI.
-   Verification that settings changes persist to `config.json`.