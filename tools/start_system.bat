@echo off
title Sentinel Safety System Launcher

echo ==============================================
echo      SENTINEL CONSTRUCTION SAFETY SYSTEM
echo ==============================================
echo.

echo [1/4] Checking Docker status...
docker info >nul 2>&1
if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Docker is not running!
    echo Please start Docker Desktop and try again.
    echo.
    pause
    exit /b
)
echo [OK] Docker is running.

echo.
echo [2/4] Starting system containers...
echo       (This may take a few minutes if running for the first time)
docker-compose up -d --build

if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Failed to start services. Check the output above.
    pause
    exit /b
)

echo.
echo [3/4] Waiting for services to initialize...
timeout /t 20 /nobreak >nul

echo.
echo [4/4] Opening Dashboard...
start http://localhost:3001

echo.
echo [SUCCESS] System is running!
echo You can manage the system via the Web Dashboard.
echo To stop the system, run stop_system.bat
echo.
pause
