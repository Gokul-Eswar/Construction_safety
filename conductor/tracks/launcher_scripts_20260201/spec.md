# Specification: System Launcher Scripts

## Context
The system is now dockerized, but requiring users to run `docker-compose` commands is poor UX. We need native scripts (`.bat` for Windows, `.sh` for Linux) to abstract this complexity.

## Goals
1.  **Simple Startup:** A `start_system` script that:
    -   Checks for Docker.
    -   Starts the containers.
    -   Opens the web browser to the dashboard.
2.  **Simple Shutdown:** A `stop_system` script to gracefully stop containers.

## Requirements
-   **Platform:** Windows (`.bat`) is priority, but Linux (`.sh`) is good for the "Edge Device" context.
-   **Port:** Web UI is on port 3001.
-   **Feedback:** Scripts should provide simple "Echo" messages to tell the user what is happening.

## Deliverables
-   `start_system.bat`
-   `stop_system.bat`
-   `start_system.sh`
-   `stop_system.sh`