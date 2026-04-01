#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${PROJECT_ROOT}" || exit 1

echo "Stopping services..."
if docker compose version > /dev/null 2>&1; then
    docker compose down
elif command -v docker-compose &> /dev/null; then
    docker-compose down
else
    echo "[ERROR] Docker Compose is not installed."
    exit 1
fi

if [ $? -ne 0 ]; then
    echo "[ERROR] Failed to stop services. Ensure Docker is running."
    exit 1
fi

echo "[SUCCESS] System stopped successfully."
