@echo off
setlocal

echo [Test Runner] Starting rigorous test execution...

:: Create build directory if it doesn't exist
if not exist build_test (
    mkdir build_test
)

cd build_test

:: Configure CMake with strict flags (already in CMakeLists.txt)
echo [Test Runner] Configuring CMake...
cmake .. -G "Visual Studio 17 2022" -A x64
if %errorlevel% neq 0 (
    echo [Test Runner] CMake configuration failed!
    exit /b %errorlevel%
)

:: Build the unit tests
echo [Test Runner] Building Unit Tests...
cmake --build . --target unit_tests --config Release
if %errorlevel% neq 0 (
    echo [Test Runner] Build failed! Please check strict warnings.
    exit /b %errorlevel%
)

:: Run C++ Unit Tests
echo [Test Runner] Running C++ Unit Tests (GTest)...
ctest -C Release --output-on-failure
if %errorlevel% neq 0 (
    echo [Test Runner] C++ Unit Tests failed!
    exit /b %errorlevel%
)

:: Run Python Infra Tests (if python is available)
where python >nul 2>nul
if %errorlevel% equ 0 (
    echo [Test Runner] Running Python Infrastructure Tests...
    cd ..
    python -m pytest tests/infra
    if %errorlevel% neq 0 (
        echo [Test Runner] Python tests failed!
        exit /b %errorlevel%
    )
) else (
    echo [Test Runner] Python not found, skipping infra tests.
)

echo [Test Runner] All tests passed successfully! Rigorous quality assured.
endlocal
