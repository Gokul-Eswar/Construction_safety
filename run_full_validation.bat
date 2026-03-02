@echo off
setlocal enabledelayedexpansion

echo ========================================================
echo [Sentinel Safety] Starting Full Validation Suite
echo ========================================================

:: 1. C++ Unit Tests
echo.
echo [1/5] Running C++ Unit Tests (GTest)...
if exist build\unit_tests.exe (
    build\unit_tests.exe
    if !errorlevel! neq 0 (
        echo [FAIL] C++ Unit Tests failed.
        exit /b !errorlevel!
    )
) else (
    echo [SKIP] build\unit_tests.exe not found. Run build_engine.bat first.
)

:: 2. Python Infrastructure Tests
echo.
echo [2/5] Running Python Infrastructure Tests...
python -m pytest tests/infra
if !errorlevel! neq 0 (
    echo [FAIL] Python Infra Tests failed.
    exit /b !errorlevel!
)

:: 3. Web Backend Tests
echo.
echo [3/5] Running Web Backend Tests (Jest)...
cd web/backend
call npm test
if !errorlevel! neq 0 (
    echo [FAIL] Web Backend Tests failed.
    cd ../..
    exit /b !errorlevel!
)
cd ../..

:: 4. Web Frontend Tests
echo.
echo [4/5] Running Web Frontend Tests (Vitest)...
cd web/frontend
call npm test
if !errorlevel! neq 0 (
    echo [FAIL] Web Frontend Tests failed.
    cd ../..
    exit /b !errorlevel!
)
cd ../..

:: 5. Stress and Resilience Tests
echo.
echo [5/5] Running Stress and Resilience Tests...
python -m pytest tests/stress_resilience_suite.py
if !errorlevel! neq 0 (
    echo [FAIL] Stress Tests failed.
    exit /b !errorlevel!
)

echo.
echo ========================================================
echo [SUCCESS] All validation layers passed successfully!
echo ========================================================
endlocal
