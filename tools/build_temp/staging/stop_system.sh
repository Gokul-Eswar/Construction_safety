#!/bin/bash
echo "Stopping services..."
if command -v docker-compose &> /dev/null; then
    docker-compose down
else
    docker compose down
fi
echo "[SUCCESS] System stopped successfully."
