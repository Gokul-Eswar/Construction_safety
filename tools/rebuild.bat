@echo off
echo ==========================================
echo   SENTINEL SAFETY - REBUILD FOR LOCALHOST
echo ==========================================
echo.

echo [1/3] Stopping existing containers...
docker-compose down

echo.
echo [2/3] Building containers from latest code...
echo This may take a few minutes if engine code changed...
docker-compose build --no-cache engine web

echo.
echo [3/3] Starting system...
docker-compose up -d

echo.
echo ==========================================
echo   System is starting!
echo   Dashboard: http://localhost:3001
echo   MQTT: localhost:1883
echo ==========================================
echo.
pause
