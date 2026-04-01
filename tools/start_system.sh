#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${PROJECT_ROOT}" || exit 1

echo "=============================================="
echo "     SENTINEL CONSTRUCTION SAFETY SYSTEM"
echo "=============================================="
echo

echo "[1/4] Checking Docker status..."
if ! command -v docker &> /dev/null; then
    echo "[ERROR] Docker is not installed."
    exit 1
fi

if ! docker info > /dev/null 2>&1; then
    echo "[ERROR] Docker is not running or permission denied (try sudo)."
    exit 1
fi
echo "[OK] Docker is running."

echo
echo "[2/4] Starting system containers..."
# Try 'docker compose' (v2) first, then 'docker-compose' (v1)
if docker compose version > /dev/null 2>&1; then
    docker compose up -d --build
elif command -v docker-compose &> /dev/null; then
    docker-compose up -d --build
else
    echo "[ERROR] Docker Compose is not installed."
    exit 1
fi

if [ $? -ne 0 ]; then
    echo
    echo "[ERROR] Failed to start services."
    exit 1
fi

echo
echo "[3/4] Waiting for services to initialize..."
sleep 10

echo
echo "[4/4] Opening Dashboard..."
if command -v xdg-open &> /dev/null; then
    xdg-open http://localhost:3001
elif command -v gnome-open &> /dev/null; then
    gnome-open http://localhost:3001
else
    echo "Please open http://localhost:3001 in your browser."
fi

echo
echo "[SUCCESS] System is running!"
