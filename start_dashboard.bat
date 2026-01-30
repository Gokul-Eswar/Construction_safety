@echo off
echo Starting Construction Safety Dashboard...

cd web\backend
if not exist node_modules (
    echo Installing backend dependencies...
    call npm install
)
start "Safety Backend" npm start

cd ..\frontend
if not exist node_modules (
    echo Installing frontend dependencies...
    call npm install
)
start "Safety Frontend" npm run dev

echo Dashboard launched!
echo Backend: http://localhost:3001
echo Frontend: http://localhost:3000
pause
