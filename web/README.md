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

### Quick Start (Windows)
Simply run the batch script in the project root:
```cmd
start_dashboard.bat
```
This will install dependencies (if needed) and launch both the backend (Port 3001) and frontend (Port 3000).

### Manual Start

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
- **Daily Stats:** Count of violations today.
- **Recent Violations:** Table of the most recent safety violations with Zone Names and Confidence levels.
- **Visual Zone Editor:** Interactive tab to draw, edit, and save safety zones directly to the system configuration.
