@echo off
setlocal enabledelayedexpansion

echo ==========================================
echo   SENTINEL SAFETY - STATIC ANALYSIS
echo ==========================================
echo.

if not exist build (
    echo [1/2] Creating build directory...
    mkdir build
)

echo [1/2] Generating compilation database...
cmake -B build -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON . > NUL 2>&1

if not exist build\compile_commands.json (
    echo Error: Failed to generate compile_commands.json
    exit /b 1
)

echo [2/2] Running Static Analysis (Clang-Tidy)...

:: Define MSYS2 System Include Paths (Hardcoded for stability)
set "INCLUDES="
set "INCLUDES=!INCLUDES! --extra-arg=-isystemC:/msys64/ucrt64/include/c++/15.1.0"
set "INCLUDES=!INCLUDES! --extra-arg=-isystemC:/msys64/ucrt64/include/c++/15.1.0/x86_64-w64-mingw32"
set "INCLUDES=!INCLUDES! --extra-arg=-isystemC:/msys64/ucrt64/include/c++/15.1.0/backward"
set "INCLUDES=!INCLUDES! --extra-arg=-isystemC:/msys64/ucrt64/lib/gcc/x86_64-w64-mingw32/15.1.0/include"
set "INCLUDES=!INCLUDES! --extra-arg=-isystemC:/msys64/ucrt64/include"
set "INCLUDES=!INCLUDES! --extra-arg=-isystemC:/msys64/ucrt64/lib/gcc/x86_64-w64-mingw32/15.1.0/include-fixed"

:: Add target and GCC version macros for better compatibility with MSYS2 headers
set "INCLUDES=!INCLUDES! --extra-arg=-target --extra-arg=x86_64-w64-mingw32"
set "INCLUDES=!INCLUDES! --extra-arg=-D__GNUC__=15 --extra-arg=-D__GNUC_MINOR__=1 --extra-arg=-D__GNUC_PATCHLEVEL__=0"

:: Suppress MSYS2/GCC specific header errors
set "INCLUDES=!INCLUDES! --extra-arg=-Wno-unknown-pragmas --extra-arg=-Wno-unused-command-line-argument"

:: Analyze only project source files from the src directory
set "SOURCE_FILES="
for /f "delims=" %%f in ('dir /s /b src\*.cpp') do (
    set "SOURCE_FILES=!SOURCE_FILES! "%%f""
)

if "!SOURCE_FILES!"=="" (
    echo No source files found to analyze.
    exit /b 0
)

:: Run clang-tidy
:: --header-filter: Only analyze files in the src directory to avoid noise from dependencies in build/_deps
clang-tidy -p build !SOURCE_FILES! --extra-arg=--driver-mode=g++ !INCLUDES! --header-filter="^.*/src/.*$" > clang_tidy_report.txt 2>&1

echo.
echo ==========================================
echo   Analysis Complete. 
echo   Results saved to: clang_tidy_report.txt
echo ==========================================
echo.
