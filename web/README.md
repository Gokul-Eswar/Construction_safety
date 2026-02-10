# Construction Safety Dashboard

A web-based dashboard for monitoring safety violations in real-time.

## Architecture

- **Backend:** Node.js + Express (Port 3001)
- **Frontend:** React + Vite + Material UI (Port 3000)
- **Database:** SQLite (`../../safety_violations.db`)

## Prerequisites

- Node.js (v18+)
- NPM

## Getting Started

### Quick Start (Recommended)
The web dashboard is automatically started as part of the main system.
- **Windows:** Run `start_system.bat` in the project root.
- **Linux:** Run `./start_system.sh` in the project root.

The dashboard will be available at `http://localhost:3001`.

### Manual Development Start

If you are developing the web interface specifically and want to run it outside of Docker:

#### 1. Start the Backend
```bash
cd web/backend
npm install
node server.js
```

#### 2. Start the Frontend
```bash
cd web/frontend
npm install
npm run dev
```

## Features

- **Real-time Status:** Shows system connectivity status.
- **Live Video Feed:** Displays the processed video stream (port 8081) with real-time detection overlays.
- **Daily Stats:** Count of violations today.
- **Recent Violations:** Table of the most recent safety violations with Zone Names and Confidence levels.
- **Visual Zone Editor:** Interactive tab to draw, edit, and save safety zones directly to the system configuration.
