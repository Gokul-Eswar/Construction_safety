@echo off
echo Building Construction Safety Engine...

if not exist build mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release --parallel

if %ERRORLEVEL% EQU 0 (
    echo.
    echo Build Successful!
    echo You can now run the engine using run_engine.bat
) else (
    echo.
    echo Build Failed! Check the output above for errors.
)
pause
