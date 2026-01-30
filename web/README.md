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

### 1. Start the Backend

```bash
cd web/backend
npm install
node server.js
```

The backend runs on `http://localhost:3001`.

### 2. Start the Frontend

```bash
cd web/frontend
npm install
npm run dev
```

The dashboard will open at `http://localhost:3000`.

## Features

- **Real-time Status:** Shows if the system is online (mocked for now).
- **Daily Stats:** Count of violations today.
- **Recent Violations:** Table of the most recent safety violations with Zone Names and Confidence levels.
