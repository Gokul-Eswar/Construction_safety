# Sentinel Safety System - Tools & Launcher

## 🎯 Quick Start

**The unified launcher consolidates all operations into a single command.**

### Windows
```batch
cd tools
Sentinel.bat
```

### Linux / macOS
```bash
cd tools
chmod +x Sentinel.sh
./Sentinel.sh
```

Both launchers present an interactive menu where you can select any operation needed.

---

## 📋 Available Operations

### 1. **Start System** (Menu Option 1)
Launches a compact hybrid runtime and opens the web dashboard.
- Starts **web + MQTT** via Docker using `docker-compose.edge.yml`
- Starts the heavy **engine natively** on the host machine
- Automatically detects Docker installation
- Handles `docker compose` vs `docker-compose` fallback
- Opens http://localhost:3001 in browser

### 2. **Stop System** (Menu Option 2)
Gracefully shuts down web/mqtt containers and the native engine process.

### 3. **Full Validation** (Menu Option 3)
Comprehensive system health check:
- Configuration files present
- Dependencies installed
- Engine binary exists
- Docker available
- Web components ready

### 4. **Run Tests** (Menu Option 4)
Executes all test suites:
- C++ unit tests (GTest)
- Python infrastructure tests (pytest)
- Validation reports any failures

### 5. **Build Engine** (Menu Option 5)
Compiles the C++ engine from source:
- Auto-detects Visual Studio (Windows)
- Uses CMake Release build
- Required for hybrid runtime (engine runs locally)

### 6. **Rebuild All** (Menu Option 6)
Clean rebuild of all components:
- Removes old build artifacts
- Rebuilds engine from scratch
- Docker images rebuild on next start

### 7. **Run Demo** (Menu Option 7)
Runs the native engine in demo mode with simulation feed.

### 8. **Lint Code** (Menu Option 8)
Static code analysis with clang-tidy:
- Checks C++ source files
- Reports style and quality issues
- Requires clang-tidy installation

### 9. **Optimize System** (Menu Option 9)
Performance tuning and recommendations:
- Analyzes system resources
- Suggests memory/CPU allocation
- Cleans temporary files

---

## 🗂️ Legacy Scripts (Deprecated)

The following individual scripts are now consolidated into `Sentinel.bat` / `Sentinel.sh`:

| Old Script | Replaced By | Use Case |
|-----------|----------|----------|
| `Setup.bat` | Sentinel → Option 1 | Initial setup (now Docker-based) |
| `start_system.bat` | Sentinel → Option 1 | Start services |
| `stop_system.bat` | Sentinel → Option 2 | Stop services |
| `build_engine.bat` | Sentinel → Option 5 | Build C++ engine |
| `rebuild.bat` | Sentinel → Option 6 | Full rebuild |
| `run_tests.bat` | Sentinel → Option 4 | Run test suite |
| `run_demo.bat` | Sentinel → Option 7 | Demo mode |
| `run_full_validation.bat` | Sentinel → Option 3 | System validation |
| `lint.bat` | Sentinel → Option 8 | Code linting |
| `optimize_system.ps1` | Sentinel → Option 9 | System optimization |
| `start_system.sh` | Sentinel.sh → Option 1 | Linux/macOS start |
| `stop_system.sh` | Sentinel.sh → Option 2 | Linux/macOS stop |
| `build_installer.ps1` | Sentinel → (installer build) | Package creation |

**These scripts will be removed in v2.0. Please migrate to the unified launcher.**

---

## 💻 System Requirements

- **Windows**: PowerShell 5.1+, CMake 3.20+, Visual Studio 2022 (optional)
- **Linux/macOS**: Bash 4.0+, CMake 3.20+, GCC/Clang
- **Docker**: Docker Desktop or Docker Engine
- **Memory**: 8GB minimum (16GB recommended)
- **GPU**: NVIDIA GPU optional (for acceleration)

---

## 🐛 Troubleshooting

### Docker not found
- **Windows**: Install [Docker Desktop](https://www.docker.com/products/docker-desktop)
- **Linux**: `sudo apt install docker.io` or equivalent
- **Check**: `docker --version`

### cl.exe not found (Windows)
- Install Visual Studio with "Desktop development with C++" workload
- Or run from x64 Native Tools Command Prompt

### Python not found
- **Windows**: `winget install Python.Python.3.11` or [python.org](https://www.python.org)
- **Linux**: `sudo apt install python3 python3-pip`
- **macOS**: `brew install python3`

### Port 3001 already in use
- Change `docker-compose.yml` web service port mapping
- Or stop conflicting application on port 3001

---

## 📝 Configuration

Edit `config.json` before starting the system:

```json
{
  "streams": [
    {
      "id": "camera_01",
      "source": "rtsp://localhost:8554/simulation",
      "enabled": true
    }
  ],
  "database": "safety_violations.db",
  "mqtt": {
    "broker": "mosquitto",
    "port": 1883
  }
}
```

See `../config.json.example` for full schema.

---

## 🔗 Useful Links

- **Dashboard**: http://localhost:3001 (after start)
- **API Docs**: http://localhost:3001/api/docs
- **Config Schema**: [config.json.example](../config.json.example)
- **User Manual**: [user_manual.md](../docs/user_manual.md)

---

## 🚀 For Developers

Each operation in the launcher is modular. To extend:

1. Edit `Sentinel.bat` or `Sentinel.sh`
2. Add a new case/function
3. Call it from the main menu

Example (add to menu):
```batch
echo   [X] My New Operation
if "%choice%"=="X" call :MY_OPERATION
```

---

## ✅ Checklist

Before production deployment:

- [ ] Run full validation (`Sentinel → Option 3`)
- [ ] Run complete test suite (`Sentinel → Option 4`)
- [ ] Update `config.json` for production
- [ ] Test with real camera feed (RTSP/USB)
- [ ] Verify database logs in `safety_violations.db`
- [ ] Check Docker resource limits in `docker-compose.prod.yml`
- [ ] Document any custom modifications

---

**Version**: 1.0  
**Last Updated**: April 1, 2026  
**Status**: Single launcher replaces 12+ individual scripts
