# Plan: Repository Reorganization

## Objective
Reorganize the repository to separate source code from documentation, making it easier to navigate and share, as requested by the user.

## Proposed Structure
- `docs/` (General documentation)
  - `edge_cases.md` (Moved from root)
  - `pre_deployment_report.md` (Moved from root)
  - `user_manual.md` (Moved from root)
  - `conductor/` (Moved from root)
- `src/` (C++ Engine source code)
- `web/` (Web Dashboard source code)
- `tests/` (Testing suite)
- `tools/` (Utility scripts)
- `installer/` (Setup scripts)
- Root: `README.md`, `CMakeLists.txt`, `docker-compose.yml`, `Dockerfile.*`, `.gitignore`, `.env.example`, `config.json.example`, `.clang-tidy`.

## Implementation Steps

### 1. Documentation Move
- Create `docs/` directory.
- Move `edge_cases.md` -> `docs/edge_cases.md`.
- Move `pre_deployment_report.md` -> `docs/pre_deployment_report.md`.
- Move `user_manual.md` -> `docs/user_manual.md`.
- Move `conductor/` -> `docs/conductor/`.

### 2. Update Documentation Links
- **`README.md`**: 
    - Update `[User Manual](user_manual.md)` -> `[User Manual](docs/user_manual.md)`
    - Update `[Edge Cases](edge_cases.md)` -> `[Edge Cases](docs/edge_cases.md)`
    - Update all `conductor/` links to `docs/conductor/`
- **`docs/conductor/index.md`**:
    - Update any links that might have broken (though if they were relative `./`, they might still work, but need verification).
- **`tools/build_installer.ps1`**:
    - Update `$Includes` array: 
        - Remove `"conductor"`
        - Remove `"checklist.md"` (if it exists, though it wasn't in `ls`)
        - Remove `"readme.md"` (Wait, `readme.md` stays in root, so it stays in `$Includes`)
        - Add `"docs"` to `$Includes`.

### 4. Verification
- Verify all links in `README.md` are working.
- Verify `conductor/index.md` links are working.
- Ensure build still works (should not be affected as `src/` and `web/` are not moved).

## Verification Plan
1. Check `docs/` directory content.
2. Manually click through links in `README.md` (simulated by checking paths).
3. Run a test build using `tools/build_engine.bat`.
