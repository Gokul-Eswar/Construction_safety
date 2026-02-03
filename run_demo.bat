@echo off
title Sentinel Demo Launcher

echo =========================================
echo   SENTINEL CONSTRUCTION SAFETY - DEMO
echo =========================================

echo.
echo [1/4] Starting MQTT Broker...
start "Sentinel MQTT Broker" /min cmd /k "node web\backend\broker.js"

echo [2/4] Starting Web Backend...
set NODE_ENV=production
start "Sentinel Web Backend" /min cmd /k "cd web\backend && node server.js"

echo.
echo Waiting for services to initialize...
timeout /t 5 /nobreak >nul

echo.
echo [3/4] Starting Inference Engine...
:: Running from root so it finds config.json
start "Sentinel Engine" cmd /k "build\main_app.exe config.json"

echo.
echo [4/4] Opening Dashboard...
start http://localhost:3001

echo.
echo =========================================
echo   SYSTEM DEPLOYED
echo =========================================
echo.
echo Close the opened windows to stop the services.
pause
