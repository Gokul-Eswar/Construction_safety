# Development Environment Setup Guide

This guide walks through setting up the Construction Safety Inference System for local development.

**TL;DR (Windows):** Run `tools/Sentinel.bat` and select option 5 (Build Engine). It handles everything.

---

## Prerequisites

### Windows
- **Visual Studio 2022** (Community edition is fine) with C++ development tools
- **CMake 3.20+** (`cmake --version` to check)
- **Python 3.9+** (for backend and tests)
- **Node.js 16+** (for web frontend)
- **Docker Desktop** (for containerized services)

### macOS / Linux
- **GCC 9+** or **Clang 10+** (`gcc --version` / `clang --version`)
- **CMake 3.20+**
- **Python 3.9+**
- **Node.js 16+**
- **Docker Desktop** (optional, for containerized services)

---

## Quick Start

### Option 1: Unified Launcher (Recommended - All Platforms)

```powershell
cd "path/to/construction safety"
.\tools\Sentinel.bat
# Or on Linux/macOS:
# ./tools/Sentinel.sh

# Select option 5: Build Engine
# It will:
# - Auto-detect Visual Studio (Windows)
# - Set up the compiler environment
# - Run CMake configuration
# - Build the C++ engine
```

### Option 2: Manual Build (Windows)

1. **Initialize Visual Studio Environment:**
   ```batch
   # Find your Visual Studio installation (usually 2022)
   "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
   ```

2. **Create and Enter Build Directory:**
   ```batch
   mkdir build
   cd build
   ```

3. **Run CMake:**
   ```batch
   cmake .. -DCMAKE_BUILD_TYPE=Release
   ```

4. **Build:**
   ```batch
   cmake --build . --config Release --parallel
   ```

   Or if using Ninja:
   ```batch
   ninja -j4
   ```

---

## Troubleshooting

### Problem: `cl.exe not found` (Windows)

**Symptom:**
```
[ERROR] CMake is not configured with a C++ compiler.
cmake: No CMAKE_CXX_COMPILER could be found.
```

**Solution:**
1. Ensure Visual Studio 2019 or 2022 is installed with C++ tools
2. Run the Visual Studio installer: `"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vs_installer.exe"`
3. Click **Modify** and ensure **Desktop development with C++** is checked
4. Let the installation complete
5. Try building again with `tools/Sentinel.bat` option 5

**Manual workaround:**
```batch
# Find vcvarsall.bat (example for VS 2022)
"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

# Then build as normal
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release --parallel
```

---

### Problem: Clang-Tidy Errors (`AnalyzeTemporaryDtors` unknown key)

**Symptom:**
```
error: unknown key 'AnalyzeTemporaryDtors'
AnalyzeTemporaryDtors: false
^~~~~~~~~~~~~~~~~~~~~
Error parsing .clang-tidy: invalid argument
```

**Root Cause:** Old or mismatched clang-tidy configuration in third-party dependencies.

**Solution:**
The root `.clang-tidy` at the workspace root now overrides dependency configurations. If issues persist:

1. **Clear the build directory and rebuild:**
   ```batch
   rm -r build
   mkdir build
   cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release
   cmake --build . --config Release
   ```

2. **Check clang-tidy version:**
   ```bash
   clang-tidy --version
   ```
   Ensure it's version 14.0 or later. Older versions don't support all configuration options.

3. **Disable clang-tidy if only building:**
   ```batch
   cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_CLANG_TIDY=""
   ```

---

### Problem: CUDA Not Found

**Symptom:**
```
STATUS "CUDA Not Found. Disabling CUDA support."
```

**Solution (Optional):**
- If you have an NVIDIA GPU and want GPU acceleration, install **CUDA Toolkit 11.8+**
- If you don't have a GPU, CPU-only mode works fine (slower, but functional)
- For Docker deployment, GPU support is automatically configured if available

---

### Problem: TensorRT Not Found

**Symptom:**
```
STATUS "TensorRT Not Found. Native TRT execution will be disabled."
```

**Solution:**
- Install **TensorRT 8.6+** from NVIDIA
- Or use Docker mode which includes pre-built TensorRT
- The system falls back to ONNX inference if TensorRT unavailable

---

## Running the System

### Docker Mode (Recommended for First Run)

```powershell
.\tools\Sentinel.bat
# Select option 1: Start System
# Dashboard opens at http://localhost:3001
```

### Native Mode (After Building)

```powershell
.\tools\Sentinel.bat
# Select option 1: Start System
# If native mode is configured in config.json, it runs locally
# Dashboard opens at http://localhost:3001
```

---

## Code Quality & Static Analysis

### Run Clang-Tidy on Source Files

```bash
cd build
clang-tidy ../src/main.cpp -- -I../include -std=c++17
```

Or use the linting option:
```powershell
.\tools\Sentinel.bat
# Select option 8: Lint Code
```

**Note:** Third-party dependencies under `build/_deps/` are excluded from analysis to keep the output manageable.

---

## Configuration Files

### `.clang-tidy` (Root)
- **Purpose:** Controls static analysis checks for the entire project
- **Location:** `d:/GOKUL_ESWAR/Codebase/construction safety/.clang-tidy`
- **Key Detail:** `InheritParentConfig: false` prevents dependency configs from overriding ours
- **HeaderFilterRegex:** Excludes `/_deps/` directories to avoid dependency warnings
- **Minimum clang-tidy version:** 14.0

### `CMakeLists.txt`
- **Compiler Validation:** Early check ensures C++ compiler is available
- **Export Compile Commands:** `set(CMAKE_EXPORT_COMPILE_COMMANDS ON)` creates `compile_commands.json` for tools
- **MSVC Detection:** On Windows, automatically validates and uses MSVC
- **Clang-Tidy Integration:** Optional environment variable `CLANG_TIDY` can enable automatic analysis during build

---

## Advanced: Running Clang-Tidy on All Source Files

**Option 1: During CMake Configuration**
```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_CLANG_TIDY="clang-tidy;-checks=-*,bugprone-*,performance-*"
cmake --build .
```

**Option 2: Post-Build Analysis Script**
Create a script (e.g., `tools/run_tidy.sh`):
```bash
#!/bin/bash
cd build
for file in ../src/**/*.cpp; do
    echo "Analyzing $file..."
    clang-tidy "$file" --fix -- -I../include -std=c++17 $(cat compile_commands.json | jq -r '.[] | select(.file | contains("'$file'")) | .arguments | join(" ")')
done
```

---

## Environment Variables

| Variable | Purpose | Example |
|----------|---------|---------|
| `CONFIG_PATH` | Override config file location | `CONFIG_PATH=/etc/safety/config.json` |
| `ENGINE_EXECUTION_MODE` | Run engine as `native` or `container` | `ENGINE_EXECUTION_MODE=native` |
| `CLANG_TIDY` | Enable clang-tidy during CMake build | `CLANG_TIDY=clang-tidy -checks=...` |
| `CMAKE_BUILD_TYPE` | Release or Debug | `CMAKE_BUILD_TYPE=Debug` |

---

## Next Steps

1. **Build the engine** using `Sentinel.bat` option 5
2. **Run tests** using option 4
3. **Start the system** using option 1
4. **View the dashboard** at `http://localhost:3001`
5. **Read** [docs/user_manual.md](user_manual.md) for operational details

---

## Support

- **Build errors?** Run `tools/Sentinel.bat` option 9 for optimization recommendations
- **Need more logs?** Check `logs/` directory for system output
- **Still stuck?** Open `tools/README.md` for detailed troubleshooting

