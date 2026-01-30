@echo off
echo Starting Construction Safety Engine...

set EXE_PATH=build\Release\main_app.exe
if not exist "%EXE_PATH%" set EXE_PATH=build\Debug\main_app.exe
if not exist "%EXE_PATH%" set EXE_PATH=build\main_app.exe

if exist "%EXE_PATH%" (
    echo Found executable at: %EXE_PATH%
    "%EXE_PATH%"
) else (
    echo Error: Executable not found!
    echo Please run build_engine.bat first.
)
pause
