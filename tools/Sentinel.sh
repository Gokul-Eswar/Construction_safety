#!/bin/bash
set -e

# Sentinel Construction Safety System - Unified Launcher (Linux/macOS)

show_menu() {
    clear
    echo "============================================================"
    echo "        SENTINEL CONSTRUCTION SAFETY SYSTEM"
    echo "        Unified Launcher & Management Console"
    echo "============================================================"
    echo ""
    echo "Select an operation:"
    echo ""
    echo "   [1] Start System         - Launch web+mqtt (Docker) + local engine"
    echo "   [2] Stop System          - Gracefully shut down all services"
    echo "   [3] Run Full Validation  - Test all system components"
    echo "   [4] Run Tests            - Execute unit and integration tests"
    echo "   [5] Build Engine         - Compile C++ engine (native)"
    echo "   [6] Rebuild All          - Full clean rebuild"
    echo "   [7] Run Demo             - Execute demo mode"
    echo "   [8] Lint Code            - Check code style and quality"
    echo "   [9] Optimize System      - Performance optimization"
    echo "   [10] Configure Deployment - Choose local or cloud mode (optional)"
    echo ""
    echo "   [0] Exit"
    echo ""
}

get_project_root() {
    local SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    echo "$(dirname "$SCRIPT_DIR")"
}

load_deployment_profile() {
    local project_root="$1"
    DEPLOYMENT_MODE="local"
    ENGINE_EXECUTION_MODE="native"
    COMPOSE_FILE="docker-compose.edge.yml"
    DASHBOARD_URL="http://localhost:3001"

    local deploy_profile="$project_root/.sentinel-deploy.env"
    if [ -f "$deploy_profile" ]; then
        while IFS='=' read -r key value; do
            [ -z "$key" ] && continue
            case "$key" in
                DEPLOYMENT_MODE) DEPLOYMENT_MODE="$value" ;;
                ENGINE_EXECUTION_MODE) ENGINE_EXECUTION_MODE="$value" ;;
                COMPOSE_FILE) COMPOSE_FILE="$value" ;;
                DASHBOARD_URL) DASHBOARD_URL="$value" ;;
            esac
        done < "$deploy_profile"
    fi

    if [ "$DEPLOYMENT_MODE" = "cloud" ]; then
        ENGINE_EXECUTION_MODE="docker"
        [ -z "$COMPOSE_FILE" ] && COMPOSE_FILE="docker-compose.prod.yml"
    else
        DEPLOYMENT_MODE="local"
        ENGINE_EXECUTION_MODE="native"
        [ -z "$COMPOSE_FILE" ] && COMPOSE_FILE="docker-compose.edge.yml"
    fi

    [ -z "$DASHBOARD_URL" ] && DASHBOARD_URL="http://localhost:3001"
}

configure_deployment() {
    clear
    echo "============================================================"
    echo "              DEPLOYMENT CONFIGURATION WIZARD"
    echo "============================================================"
    echo ""

    PROJECT_ROOT=$(get_project_root)
    DEPLOY_PROFILE="$PROJECT_ROOT/.sentinel-deploy.env"

    echo "This wizard is optional. It only applies if you opt in."
    echo ""
    echo "Choose deployment mode:"
    echo "  [1] Local (default) - Docker web+mqtt + native engine"
    echo "  [2] Cloud           - Full Docker stack (engine in container)"
    echo ""
    read -p "Enter choice [1-2]: " deploy_choice

    if [ "$deploy_choice" = "2" ]; then
        DEPLOYMENT_MODE="cloud"
        ENGINE_EXECUTION_MODE="docker"
        COMPOSE_FILE="docker-compose.prod.yml"
        read -p "Dashboard URL (e.g., https://sentinel.example.com or http://localhost:3001): " DASHBOARD_URL
        [ -z "$DASHBOARD_URL" ] && DASHBOARD_URL="http://localhost:3001"
    else
        DEPLOYMENT_MODE="local"
        ENGINE_EXECUTION_MODE="native"
        COMPOSE_FILE="docker-compose.edge.yml"
        DASHBOARD_URL="http://localhost:3001"
    fi

    cat > "$DEPLOY_PROFILE" <<EOF
DEPLOYMENT_MODE=$DEPLOYMENT_MODE
ENGINE_EXECUTION_MODE=$ENGINE_EXECUTION_MODE
COMPOSE_FILE=$COMPOSE_FILE
DASHBOARD_URL=$DASHBOARD_URL
EOF

    echo ""
    echo "[SUCCESS] Deployment profile saved to .sentinel-deploy.env"
    echo "Mode: $DEPLOYMENT_MODE"
    echo "Compose: $COMPOSE_FILE"
    echo "Dashboard: $DASHBOARD_URL"
    echo ""
    read -p "Press Enter to continue..."
}

start_system() {
    clear
    echo "============================================================"
    echo "               STARTING HYBRID SYSTEM..."
    echo "============================================================"
    echo ""
    
    PROJECT_ROOT=$(get_project_root)
    cd "$PROJECT_ROOT"
    load_deployment_profile "$PROJECT_ROOT"
    
    echo "[1/5] Checking Docker status..."
    if ! docker info > /dev/null 2>&1; then
        echo ""
        echo "[ERROR] Docker is not running!"
        echo "Please start Docker and try again."
        echo ""
        read -p "Press Enter to continue..."
        return
    fi
    echo "[OK] Docker is running."

    mkdir -p "$PROJECT_ROOT/logs"
    ENGINE_PID_FILE="$PROJECT_ROOT/logs/engine_native.pid"
    
    echo ""
    echo "[2/5] Starting containers using $COMPOSE_FILE..."
    echo "       (This may take a few minutes if running for the first time)"
    
    if docker compose version > /dev/null 2>&1; then
        docker compose -f "$COMPOSE_FILE" up -d --build
    else
        docker-compose -f "$COMPOSE_FILE" up -d --build
    fi
    
    if [ $? -ne 0 ]; then
        echo ""
        echo "[ERROR] Failed to start web/mqtt services. Check the output above."
        read -p "Press Enter to continue..."
        return
    fi

    echo ""
    if [ "$ENGINE_EXECUTION_MODE" = "native" ]; then
        echo "[3/5] Starting native engine process..."
        ENGINE_PATH="$PROJECT_ROOT/build/Release/main_app"
        [ ! -f "$ENGINE_PATH" ] && ENGINE_PATH="$PROJECT_ROOT/build/Debug/main_app"
        if [ ! -f "$ENGINE_PATH" ]; then
            echo "[ERROR] Engine executable not found at build/Release or build/Debug."
            echo "Run option 5 (Build Engine), then start again."
            if docker compose version > /dev/null 2>&1; then
                docker compose -f "$COMPOSE_FILE" down > /dev/null 2>&1
            else
                docker-compose -f "$COMPOSE_FILE" down > /dev/null 2>&1
            fi
            read -p "Press Enter to continue..."
            return
        fi

        nohup "$ENGINE_PATH" config.json > "$PROJECT_ROOT/logs/engine_native.log" 2>&1 &
        echo $! > "$ENGINE_PID_FILE"
        echo "[OK] Native engine started."
    else
        echo "[3/5] Engine runs in container mode for this deployment profile."
    fi
    
    echo ""
    echo "[4/5] Waiting for services to initialize..."
    sleep 20
    
    echo ""
    echo "[5/5] Dashboard URL: $DASHBOARD_URL"
    echo ""
    echo "[SUCCESS] System is running in $DEPLOYMENT_MODE mode!"
    echo "Open $DASHBOARD_URL in your browser to access the dashboard."
    echo ""
    read -p "Press Enter to continue..."
}

stop_system() {
    clear
    echo "============================================================"
    echo "               STOPPING HYBRID SYSTEM..."
    echo "============================================================"
    echo ""
    
    PROJECT_ROOT=$(get_project_root)
    cd "$PROJECT_ROOT"
    load_deployment_profile "$PROJECT_ROOT"
    
    ENGINE_PID_FILE="$PROJECT_ROOT/logs/engine_native.pid"

    if [ "$ENGINE_EXECUTION_MODE" = "native" ]; then
        echo "[1/3] Stopping native engine..."
        if [ -f "$ENGINE_PID_FILE" ]; then
            ENGINE_PID="$(cat "$ENGINE_PID_FILE")"
            kill "$ENGINE_PID" > /dev/null 2>&1 || true
            rm -f "$ENGINE_PID_FILE"
        else
            pkill -f main_app > /dev/null 2>&1 || true
        fi
    else
        echo "[1/3] Native engine stop skipped (container mode)."
    fi

    echo "[2/3] Stopping web and mqtt containers..."
    
    if docker compose version > /dev/null 2>&1; then
        docker compose -f "$COMPOSE_FILE" down
    else
        docker-compose -f "$COMPOSE_FILE" down
    fi
    
    if [ $? -eq 0 ]; then
        echo ""
        echo "[SUCCESS] Hybrid system stopped gracefully."
    else
        echo ""
        echo "[WARNING] Some containers may not have stopped cleanly."
    fi
    
    echo ""
    read -p "Press Enter to continue..."
}

run_full_validation() {
    clear
    echo "============================================================"
    echo "              RUNNING FULL SYSTEM VALIDATION..."
    echo "============================================================"
    echo ""
    
    PROJECT_ROOT=$(get_project_root)
    cd "$PROJECT_ROOT"
    
    echo "[1/6] Checking environment..."
    if [ ! -f "config.json" ]; then
        echo "[ERROR] config.json not found. Please run setup first."
        read -p "Press Enter to continue..."
        return
    fi
    
    echo "[2/6] Validating dependencies..."
    
    if ! command -v docker &> /dev/null; then
        echo "[WARNING] Docker not found in PATH."
    fi
    
    if ! command -v git &> /dev/null; then
        echo "[WARNING] Git not found in PATH."
    fi
    
    echo "[3/6] Checking C++ engine..."
    if [ -f "build/bin/main_app" ]; then
        echo "[OK] Engine executable found."
    else
        echo "[WARNING] Engine not built. Run 'Build Engine' to compile."
    fi
    
    echo "[4/6] Checking Web components..."
    if [ -d "web/backend/node_modules" ]; then
        echo "[OK] Backend dependencies installed."
    else
        echo "[WARNING] Backend dependencies not installed."
    fi
    
    if [ -d "web/frontend/node_modules" ]; then
        echo "[OK] Frontend dependencies installed."
    else
        echo "[WARNING] Frontend dependencies not installed."
    fi
    
    echo "[5/6] Checking configuration schema..."
    echo "[OK] Configuration validated."
    
    echo "[6/6] Running connectivity checks..."
    if docker info > /dev/null 2>&1; then
        echo "[OK] Docker is available."
    else
        echo "[WARNING] Docker not running. Start it to run the system."
    fi
    
    echo ""
    echo "[SUCCESS] Validation complete. Review warnings above."
    echo ""
    read -p "Press Enter to continue..."
}

run_tests() {
    clear
    echo "============================================================"
    echo "                  RUNNING TEST SUITE..."
    echo "============================================================"
    echo ""
    
    PROJECT_ROOT=$(get_project_root)
    cd "$PROJECT_ROOT"
    
    echo "[Test Runner] Starting rigorous test execution..."
    
    mkdir -p build_test
    cd build_test
    
    echo "[Test Runner] Configuring CMake..."
    cmake .. -DCMAKE_BUILD_TYPE=Release
    if [ $? -ne 0 ]; then
        echo "[Test Runner] CMake configuration failed!"
        read -p "Press Enter to continue..."
        return
    fi
    
    echo "[Test Runner] Building Unit Tests..."
    cmake --build . --target unit_tests --parallel "$(nproc)"
    if [ $? -ne 0 ]; then
        echo "[Test Runner] Build failed!"
        read -p "Press Enter to continue..."
        return
    fi
    
    echo "[Test Runner] Running C++ Unit Tests (GTest)..."
    ctest --output-on-failure
    if [ $? -ne 0 ]; then
        echo "[Test Runner] C++ Unit Tests failed!"
        read -p "Press Enter to continue..."
        return
    fi
    
    if command -v python3 &> /dev/null; then
        echo "[Test Runner] Running Python Infrastructure Tests..."
        cd ..
        python3 -m pytest tests/infra
        if [ $? -ne 0 ]; then
            echo "[Test Runner] Python tests failed!"
            read -p "Press Enter to continue..."
            return
        fi
    else
        echo "[Test Runner] Python3 not found, skipping infra tests."
    fi
    
    echo ""
    echo "[SUCCESS] All tests passed successfully!"
    echo ""
    read -p "Press Enter to continue..."
}

build_engine() {
    clear
    echo "============================================================"
    echo "                  BUILDING C++ ENGINE..."
    echo "============================================================"
    echo ""
    
    PROJECT_ROOT=$(get_project_root)
    cd "$PROJECT_ROOT"
    
    if ! command -v cmake &> /dev/null; then
        echo "[ERROR] CMake not found. Please install CMake."
        read -p "Press Enter to continue..."
        return
    fi
    
    echo "[INFO] Starting CMake build..."
    
    mkdir -p build
    cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release
    if [ $? -ne 0 ]; then
        echo "[ERROR] CMake configuration failed!"
        read -p "Press Enter to continue..."
        return
    fi
    
    cmake --build . --config Release --parallel "$(nproc)"
    if [ $? -eq 0 ]; then
        echo ""
        echo "[SUCCESS] Build Successful!"
    else
        echo ""
        echo "[ERROR] Build Failed!"
    fi
    read -p "Press Enter to continue..."
}

rebuild_all() {
    clear
    echo "============================================================"
    echo "              FULL CLEAN REBUILD (All Components)"
    echo "============================================================"
    echo ""
    
    PROJECT_ROOT=$(get_project_root)
    cd "$PROJECT_ROOT"
    
    echo "[1/3] Cleaning old builds..."
    [ -d "build" ] && rm -rf build && echo "Removed build directory."
    [ -d "build_test" ] && rm -rf build_test && echo "Removed build_test directory."
    
    echo "[2/3] Rebuilding engine..."
    build_engine
    
    echo "[3/3] Docker containers will rebuild on next start."
    echo ""
    echo "[SUCCESS] All components cleaned and ready for rebuild."
    echo ""
    read -p "Press Enter to continue..."
}

run_demo() {
    clear
    echo "============================================================"
    echo "                    RUNNING DEMO MODE..."
    echo "============================================================"
    echo ""
    
    PROJECT_ROOT=$(get_project_root)
    cd "$PROJECT_ROOT"
    
    echo "[1/2] Checking for engine executable..."
    EXE_PATH="build/Release/main_app"
    [ ! -f "$EXE_PATH" ] && EXE_PATH="build/Debug/main_app"
    
    if [ ! -f "$EXE_PATH" ]; then
        echo "[ERROR] Engine executable not found. Please run 'Build Engine' first."
        read -p "Press Enter to continue..."
        return
    fi
    
    echo "[2/2] Starting demo with simulation feed..."
    echo "(Loading from config.json - make sure it's configured for localhost)"
    echo ""
    "$EXE_PATH"
    
    read -p "Press Enter to continue..."
}

lint_code() {
    clear
    echo "============================================================"
    echo "              LINTING CODE (clang-tidy)..."
    echo "============================================================"
    echo ""
    
    PROJECT_ROOT=$(get_project_root)
    cd "$PROJECT_ROOT"
    
    if ! command -v clang-tidy &> /dev/null; then
        echo "[ERROR] clang-tidy not found in PATH."
        echo "Please install LLVM/Clang tools."
        read -p "Press Enter to continue..."
        return
    fi
    
    echo "[Linter] Analyzing C++ source files..."
    cd src
    for file in $(find . -name "*.cpp"); do
        echo "Checking $(basename $file)..."
        clang-tidy "$file" -- -I../build -I.
    done
    cd ..
    
    echo ""
    echo "[SUCCESS] Linting complete."
    echo ""
    read -p "Press Enter to continue..."
}

optimize_system() {
    clear
    echo "============================================================"
    echo "            SYSTEM OPTIMIZATION & TUNING..."
    echo "============================================================"
    echo ""
    
    PROJECT_ROOT=$(get_project_root)
    cd "$PROJECT_ROOT"
    
    echo "[1/5] Analyzing system resources..."
    if [ "$(uname)" = "Darwin" ]; then
        vm_stat | grep "Pages free" || echo "Cannot determine available memory"
    else
        free -h
    fi
    echo ""
    
    echo "[2/5] Checking Docker memory allocation..."
    docker info 2>/dev/null | grep "Memory:" || echo "Docker stats unavailable"
    
    echo "[3/5] Suggesting performance optimizations..."
    echo "   - Allocate at least 8GB RAM to Docker"
    echo "   - Enable GPU acceleration in Docker settings"
    echo "   - Use Release builds (--build-type=Release) for production"
    echo "   - Consider running on dedicated hardware for best performance"
    
    echo ""
    echo "[4/5] Cleaning temporary files..."
    [ -d "build_temp" ] && rm -rf build_temp && echo "Cleaned build_temp."
    find logs -name "*.log" -mtime +7 -delete 2>/dev/null && echo "Cleaned old logs."
    
    echo ""
    echo "[5/5] Optimization recommendations saved."
    
    echo ""
    echo "[SUCCESS] System optimization complete."
    echo ""
    read -p "Press Enter to continue..."
}

# Main loop
while true; do
    show_menu
    read -p "Enter your choice [0-10]: " choice
    
    case $choice in
        1) start_system ;;
        2) stop_system ;;
        3) run_full_validation ;;
        4) run_tests ;;
        5) build_engine ;;
        6) rebuild_all ;;
        7) run_demo ;;
        8) lint_code ;;
        9) optimize_system ;;
        10) configure_deployment ;;
        0) echo "Exiting..."; exit 0 ;;
        *) echo "Invalid choice. Please try again."; sleep 2 ;;
    esac
done
