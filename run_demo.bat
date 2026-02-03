@echo off
title Sentinel Demo Launcher

echo =========================================
echo   SENTINEL CONSTRUCTION SAFETY - DEMO
echo =========================================

echo.
echo [1/5] Checking Web Dependencies...

:: Backend Deps
if not exist "web\backend\node_modules" (
    echo     Installing Backend Dependencies...
    pushd web\backend
    call npm install
    popd
)

:: Frontend Deps & Build
if not exist "web\frontend\node_modules" (
    echo     Installing Frontend Dependencies...
    pushd web\frontend
    call npm install
    popd
)

if not exist "web\frontend\dist" (
    echo     Building Frontend (this may take a minute)...
    pushd web\frontend
    call npm run build
    popd
)

echo.
echo [2/5] Starting Services...

echo     Starting MQTT Broker...
start "Sentinel MQTT Broker" /min cmd /k "node web\backend\broker.js"

echo     Starting Web Backend...
set NODE_ENV=production
start "Sentinel Web Backend" /min cmd /k "cd web\backend && node server.js"

echo.
echo Waiting for services to initialize...
timeout /t 5 /nobreak >nul

echo.
echo [3/5] Locating Inference Engine...
set EXE_PATH=build\Release\main_app.exe
if not exist "%EXE_PATH%" set EXE_PATH=build\Debug\main_app.exe
if not exist "%EXE_PATH%" set EXE_PATH=build\main_app.exe

if exist "%EXE_PATH%" (
    echo [4/5] Starting Inference Engine...
    echo     Found at: %EXE_PATH%
    start "Sentinel Engine" cmd /k "%EXE_PATH% config.json"
) else (
    echo [WARNING] Engine executable not found!
    echo Running in Web-Only mode. Run build_engine.bat to build the C++ engine.
)

echo.
echo [5/5] Opening Dashboard...
start http://localhost:3001

echo.
echo =========================================
echo   SYSTEM DEPLOYED
echo =========================================
echo.
echo Close the opened windows to stop the services.
pause
