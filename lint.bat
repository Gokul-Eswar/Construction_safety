@echo off
setlocal enabledelayedexpansion

echo Generating compilation database...
cmake -B build -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON .

if not exist build\compile_commands.json (
    echo Error: Failed to generate compile_commands.json
    exit /b 1
)

echo Running Static Analysis (Clang-Tidy)...
echo Analyzed files:
set "SOURCE_FILES="
for /r src %%f in (*.cpp) do (
    set "SOURCE_FILES=!SOURCE_FILES! "%%f""
)

clang-tidy -p build !SOURCE_FILES! --extra-arg=--driver-mode=g++ --header-filter=src/.*

echo Analysis Complete.
pause
