#!/bin/bash
echo "Stopping services..."
if docker compose version > /dev/null 2>&1; then
    docker compose down
elif command -v docker-compose &> /dev/null; then
    docker-compose down
else
    echo "[ERROR] Docker Compose is not installed."
    exit 1
fi
echo "[SUCCESS] System stopped successfully."
