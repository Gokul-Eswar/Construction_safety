@echo off
title Sentinel Safety System Shutdown

echo ==============================================
echo      SENTINEL CONSTRUCTION SAFETY SYSTEM
echo ==============================================
echo.

echo Stopping services...
docker-compose down

echo.
echo [SUCCESS] System stopped successfully.
pause
