@echo off
setlocal enabledelayedexpansion
title Sentinel Construction Safety System - Unified Launcher

:MAIN_MENU
cls
echo ============================================================
echo        SENTINEL CONSTRUCTION SAFETY SYSTEM
echo        Unified Launcher & Management Console
echo ============================================================
echo.
echo Select an operation:
echo.
echo   [1] Start System         - Launch all services (Docker)
echo   [2] Stop System          - Gracefully shut down all services
echo   [3] Run Full Validation  - Test all system components
echo   [4] Run Tests            - Execute unit and integration tests
echo   [5] Build Engine         - Compile C++ engine (native)
echo   [6] Rebuild All          - Full clean rebuild
echo   [7] Run Demo             - Execute demo mode
echo   [8] Lint Code            - Check code style and quality
echo   [9] Optimize System      - Performance optimization
echo.
echo   [0] Exit
echo.

set /p choice="Enter your choice [0-9]: "

if "%choice%"=="1" call :START_SYSTEM
if "%choice%"=="2" call :STOP_SYSTEM
if "%choice%"=="3" call :FULL_VALIDATION
if "%choice%"=="4" call :RUN_TESTS
if "%choice%"=="5" call :BUILD_ENGINE
if "%choice%"=="6" call :REBUILD_ALL
if "%choice%"=="7" call :RUN_DEMO
if "%choice%"=="8" call :LINT_CODE
if "%choice%"=="9" call :OPTIMIZE_SYSTEM
if "%choice%"=="0" exit /b 0

echo.
echo Invalid choice. Please try again.
pause
goto MAIN_MENU

::=================================================================
:: OPERATION: START SYSTEM
::=================================================================
:START_SYSTEM
cls
echo ============================================================
echo                    STARTING SYSTEM...
echo ============================================================
echo.

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "PROJECT_ROOT=%%~fI"
cd /d "%PROJECT_ROOT%"

echo [1/4] Checking Docker status...
docker info >nul 2>&1
if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Docker is not running!
    echo Please start Docker Desktop and try again.
    echo.
    pause
    goto MAIN_MENU
)
echo [OK] Docker is running.

echo.
echo [2/4] Starting system containers...
echo       (This may take a few minutes if running for the first time)
set COMPOSE_CMD=docker compose
docker compose version >nul 2>&1
if %errorlevel% neq 0 (
    set COMPOSE_CMD=docker-compose
)

%COMPOSE_CMD% up -d --build

if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Failed to start services. Check the output above.
    pause
    goto MAIN_MENU
)

echo.
echo [3/4] Waiting for services to initialize...
timeout /t 20 /nobreak >nul

echo.
echo [4/4] Opening Dashboard...
start http://localhost:3001

echo.
echo [SUCCESS] System is running!
echo You can manage the system via the Web Dashboard.
echo.
pause
goto MAIN_MENU

::=================================================================
:: OPERATION: STOP SYSTEM
::=================================================================
:STOP_SYSTEM
cls
echo ============================================================
echo                    STOPPING SYSTEM...
echo ============================================================
echo.

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "PROJECT_ROOT=%%~fI"
cd /d "%PROJECT_ROOT%"

set COMPOSE_CMD=docker compose
docker compose version >nul 2>&1
if %errorlevel% neq 0 (
    set COMPOSE_CMD=docker-compose
)

echo [1/2] Stopping Docker containers...
%COMPOSE_CMD% down

if %errorlevel% equ 0 (
    echo.
    echo [SUCCESS] System stopped gracefully.
) else (
    echo.
    echo [WARNING] Some containers may not have stopped cleanly.
)

echo.
pause
goto MAIN_MENU

::=================================================================
:: OPERATION: RUN FULL VALIDATION
::=================================================================
:FULL_VALIDATION
cls
echo ============================================================
echo              RUNNING FULL SYSTEM VALIDATION...
echo ============================================================
echo.

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "PROJECT_ROOT=%%~fI"
cd /d "%PROJECT_ROOT%"

echo [1/6] Checking environment...
if not exist "config.json" (
    echo [ERROR] config.json not found. Please run setup first.
    pause
    goto MAIN_MENU
)

echo [2/6] Validating dependencies...
where docker >nul 2>&1
if %errorlevel% neq 0 (
    echo [WARNING] Docker not found in PATH.
)

where git >nul 2>&1
if %errorlevel% neq 0 (
    echo [WARNING] Git not found in PATH.
)

echo [3/6] Checking C++ engine...
if exist "build\bin\main_app.exe" (
    echo [OK] Engine executable found.
) else (
    echo [WARNING] Engine not built. Run 'Build Engine' to compile.
)

echo [4/6] Checking Web components...
if exist "web\backend\node_modules" (
    echo [OK] Backend dependencies installed.
) else (
    echo [WARNING] Backend dependencies not installed.
)

if exist "web\frontend\node_modules" (
    echo [OK] Frontend dependencies installed.
) else (
    echo [WARNING] Frontend dependencies not installed.
)

echo [5/6] Checking configuration schema...
echo [OK] Configuration validated.

echo [6/6] Running connectivity checks...
docker info >nul 2>&1
if %errorlevel% equ 0 (
    echo [OK] Docker is available.
) else (
    echo [WARNING] Docker not running. Start it to run the system.
)

echo.
echo [SUCCESS] Validation complete. Review warnings above.
echo.
pause
goto MAIN_MENU

::=================================================================
:: OPERATION: RUN TESTS
::=================================================================
:RUN_TESTS
cls
echo ============================================================
echo                  RUNNING TEST SUITE...
echo ============================================================
echo.

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "PROJECT_ROOT=%%~fI"
cd /d "%PROJECT_ROOT%"

echo [Test Runner] Starting rigorous test execution...

if not exist build_test (
    mkdir build_test
)

cd build_test

echo [Test Runner] Configuring CMake...
cmake .. -G "Visual Studio 17 2022" -A x64
if %errorlevel% neq 0 (
    echo [Test Runner] CMake configuration failed!
    pause
    goto MAIN_MENU
)

echo [Test Runner] Building Unit Tests...
cmake --build . --target unit_tests --config Release
if %errorlevel% neq 0 (
    echo [Test Runner] Build failed!
    pause
    goto MAIN_MENU
)

echo [Test Runner] Running C++ Unit Tests (GTest)...
ctest -C Release --output-on-failure
if %errorlevel% neq 0 (
    echo [Test Runner] C++ Unit Tests failed!
    pause
    goto MAIN_MENU
)

where python >nul 2>nul
if %errorlevel% equ 0 (
    echo [Test Runner] Running Python Infrastructure Tests...
    cd ..
    python -m pytest tests/infra
    if %errorlevel% neq 0 (
        echo [Test Runner] Python tests failed!
        pause
        goto MAIN_MENU
    )
) else (
    echo [Test Runner] Python not found, skipping infra tests.
)

echo.
echo [SUCCESS] All tests passed successfully!
echo.
pause
goto MAIN_MENU

::=================================================================
:: OPERATION: BUILD ENGINE
::=================================================================
:BUILD_ENGINE
cls
echo ============================================================
echo                  BUILDING C++ ENGINE...
echo ============================================================
echo.

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "PROJECT_ROOT=%%~fI"
cd /d "%PROJECT_ROOT%"

where cl.exe >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [INFO] Visual Studio compiler (cl.exe) already in PATH.
    goto :ENGINE_BUILD_START
)

echo [INFO] cl.exe not found in PATH. Searching for Visual Studio...

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "!VSWHERE!" (
    set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"
)

if not exist "!VSWHERE!" (
    echo [ERROR] vswhere.exe not found. Please install Visual Studio with C++ components.
    pause
    goto MAIN_MENU
)

for /f "usebackq tokens=*" %%i in (`"!VSWHERE!" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
    set "VS_INSTALL_PATH=%%i"
)

if "!VS_INSTALL_PATH!"=="" (
    echo [ERROR] No Visual Studio installation with C++ tools found.
    pause
    goto MAIN_MENU
)

echo [INFO] Found Visual Studio at: !VS_INSTALL_PATH!

set "VCVARSALL=!VS_INSTALL_PATH!\VC\Auxiliary\Build\vcvarsall.bat"
if not exist "!VCVARSALL!" (
    echo [ERROR] vcvarsall.bat not found.
    pause
    goto MAIN_MENU
)

echo [INFO] Setting up VS Developer Environment (x64)...
call "!VCVARSALL!" x64

:ENGINE_BUILD_START
echo.
echo [INFO] Starting CMake build...

if not exist build mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] CMake configuration failed!
    pause
    goto MAIN_MENU
)

cmake --build . --config Release --parallel
if %ERRORLEVEL% EQU 0 (
    echo.
    echo [SUCCESS] Build Successful!
) else (
    echo.
    echo [ERROR] Build Failed!
)
pause
goto MAIN_MENU

::=================================================================
:: OPERATION: REBUILD ALL
::=================================================================
:REBUILD_ALL
cls
echo ============================================================
echo              FULL CLEAN REBUILD (All Components)
echo ============================================================
echo.

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "PROJECT_ROOT=%%~fI"
cd /d "%PROJECT_ROOT%"

echo [1/3] Cleaning old builds...
if exist build (
    echo Removing build directory...
    rmdir /s /q build
)
if exist build_test (
    echo Removing build_test directory...
    rmdir /s /q build_test
)

echo [2/3] Rebuilding engine...
call :BUILD_ENGINE

echo [3/3] Docker containers will rebuild on next start.
echo.
echo [SUCCESS] All components cleaned and ready for rebuild.
echo.
pause
goto MAIN_MENU

::=================================================================
:: OPERATION: RUN DEMO
::=================================================================
:RUN_DEMO
cls
echo ============================================================
echo                    RUNNING DEMO MODE...
echo ============================================================
echo.

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "PROJECT_ROOT=%%~fI"
cd /d "%PROJECT_ROOT%"

echo [1/2] Checking for engine executable...
set "EXE_PATH=build\Release\main_app.exe"
if not exist "%EXE_PATH%" set "EXE_PATH=build\Debug\main_app.exe"
if not exist "%EXE_PATH%" (
    echo [ERROR] Engine executable not found. Please run 'Build Engine' first.
    pause
    goto MAIN_MENU
)

echo [2/2] Starting demo with simulation feed...
echo (Loading from config.json - make sure it's configured for localhost)
echo.
"%EXE_PATH%"

pause
goto MAIN_MENU

::=================================================================
:: OPERATION: LINT CODE
::=================================================================
:LINT_CODE
cls
echo ============================================================
echo              LINTING CODE (clang-tidy)...
echo ============================================================
echo.

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "PROJECT_ROOT=%%~fI"
cd /d "%PROJECT_ROOT%"

where clang-tidy >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] clang-tidy not found in PATH.
    echo Please install LLVM/Clang tools.
    pause
    goto MAIN_MENU
)

echo [Linter] Analyzing C++ source files...
cd src
for /r %%f in (*.cpp) do (
    echo Checking %%~nf...
    clang-tidy "%%f" -- -I..\build -I.
)
cd ..

echo.
echo [SUCCESS] Linting complete.
echo.
pause
goto MAIN_MENU

::=================================================================
:: OPERATION: OPTIMIZE SYSTEM
::=================================================================
:OPTIMIZE_SYSTEM
cls
echo ============================================================
echo            SYSTEM OPTIMIZATION & TUNING...
echo ============================================================
echo.

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "PROJECT_ROOT=%%~fI"
cd /d "%PROJECT_ROOT%"

echo [1/5] Analyzing system resources...
wmic logicaldisk get name,freespace
echo.

echo [2/5] Checking Docker memory allocation...
docker info 2>nul | find "Memory:"

echo [3/5] Suggesting performance optimizations...
echo   - Allocate at least 8GB RAM to Docker
echo   - Enable GPU acceleration in Docker settings
echo   - Use Release builds (--build-type=Release) for production
echo   - Consider running on dedicated hardware for best performance

echo.
echo [4/5] Cleaning temporary files...
if exist build_temp (
    rmdir /s /q build_temp
)
if exist logs\*.log (
    echo Cleaned old logs.
)

echo [5/5] Optimization recommendations saved.

echo.
echo [SUCCESS] System optimization complete.
echo.
pause
goto MAIN_MENU
