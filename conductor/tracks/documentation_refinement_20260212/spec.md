# Specification: Documentation Refinement

## Context
The system has recently added "Features 2.0" (Auto-Healing, Cloud Sync, Robust RTSP) and the `pre_deployment_report` identified some native build pitfalls. The documentation needs to be refined to include these details.

## Goals
1.  **Refine `readme.md`:** Ensure developers know the prerequisites for manual builds (VS Build Tools).
2.  **Enhance `user_manual.md`:** Explain the new robust features to the end user.
3.  **Update `edge_cases.md`:** Document the build environment constraints.

## Requirements
-   **Auto-Healing:** Explain that the system monitors the engine and restarts it if it freezes.
-   **Cloud Sync:** Explain that logs are synced to `safety/cloud_sync` and status is shown on the dashboard.
-   **Build Tools:** Explicitly state `cl.exe` requirement for Windows native builds.

## Deliverables
-   Updated `readme.md`
-   Updated `user_manual.md`
-   Updated `edge_cases.md`
