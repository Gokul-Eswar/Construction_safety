@echo off
setlocal enabledelayedexpansion

echo ==========================================
echo   SENTINEL SAFETY - ENGINE BUILD
echo ==========================================
echo.

:: 1. Check if compiler is already available
where cl.exe >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [INFO] Visual Studio compiler (cl.exe) already in PATH.
    goto :START_BUILD
)

echo [INFO] cl.exe not found in PATH. Searching for Visual Studio...

:: 2. Search for vswhere.exe
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "!VSWHERE!" (
    set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"
)

if not exist "!VSWHERE!" (
    echo [ERROR] vswhere.exe not found. Please install Visual Studio with C++ components.
    echo.
    echo If Visual Studio is installed, make sure it has "Desktop development with C++" workload.
    pause
    exit /b 1
)

:: 3. Find VS installation path
for /f "usebackq tokens=*" %%i in (`"!VSWHERE!" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
    set "VS_INSTALL_PATH=%%i"
)

if "!VS_INSTALL_PATH!"=="" (
    echo [ERROR] No Visual Studio installation with C++ tools found.
    echo Please install Visual Studio with "Desktop development with C++" workload.
    pause
    exit /b 1
)

echo [INFO] Found Visual Studio at: !VS_INSTALL_PATH!

:: 4. Locate vcvarsall.bat
set "VCVARSALL=!VS_INSTALL_PATH!\VC\Auxiliary\Build\vcvarsall.bat"
if not exist "!VCVARSALL!" (
    echo [ERROR] vcvarsall.bat not found at !VCVARSALL!
    pause
    exit /b 1
)

:: 5. Set up environment
echo [INFO] Setting up VS Developer Environment (x64)...
call "!VCVARSALL!" x64

:START_BUILD
echo.
echo [INFO] Starting CMake build...

if not exist build mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] CMake configuration failed!
    pause
    exit /b 1
)

cmake --build . --config Release --parallel
if %ERRORLEVEL% EQU 0 (
    echo.
    echo [SUCCESS] Build Successful!
    echo You can now run the engine using run_engine.bat
) else (
    echo.
    echo [ERROR] Build Failed! Check the output above for errors.
)

pause
