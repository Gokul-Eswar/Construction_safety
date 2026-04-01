@echo off
title Sentinel Safety System Shutdown

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "PROJECT_ROOT=%%~fI"
cd /d "%PROJECT_ROOT%"

echo ==============================================
echo      SENTINEL CONSTRUCTION SAFETY SYSTEM
echo ==============================================
echo.

echo Stopping services...
set COMPOSE_CMD=docker compose
docker compose version >nul 2>&1
if %errorlevel% neq 0 (
	set COMPOSE_CMD=docker-compose
)

%COMPOSE_CMD% down

echo.
echo [SUCCESS] System stopped successfully.
pause
