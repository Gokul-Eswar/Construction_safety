# Optimization Script for Sentinel Safety System
# Run this script as Administrator for best results.

Write-Host "==============================================" -ForegroundColor Cyan
Write-Host "     SENTINEL SYSTEM OPTIMIZER" -ForegroundColor Cyan
Write-Host "==============================================" -ForegroundColor Cyan
Write-Host ""

# 1. Hardware Check
Write-Host "[1/4] Checking Hardware..." -ForegroundColor Yellow
if (Get-Command nvidia-smi -ErrorAction SilentlyContinue) {
    Write-Host "  [OK] NVIDIA GPU detected." -ForegroundColor Green
    $gpu_info = nvidia-smi --query-gpu=name,memory.total --format=csv,noheader
    Write-Host "  Details: $gpu_info" -ForegroundColor Gray
} else {
    Write-Host "  [WARN] NVIDIA GPU not found! System will run in CPU fallback mode (slower)." -ForegroundColor Red
}

# 2. Windows Power Plan Optimization
Write-Host ""
Write-Host "[2/4] Optimizing Power Settings..." -ForegroundColor Yellow
try {
    # High Performance GUID: 8c5e7fda-e8bf-4a96-9a85-a6e23a8c635c
    powercfg /setactive 8c5e7fda-e8bf-4a96-9a85-a6e23a8c635c
    Write-Host "  [OK] Power Plan set to High Performance." -ForegroundColor Green
} catch {
    Write-Host "  [WARN] Could not set Power Plan automatically. Please ensure 'High Performance' is selected in Windows Settings." -ForegroundColor Yellow
}

# 3. Config Optimization
Write-Host ""
Write-Host "[3/4] Tuning Configuration..." -ForegroundColor Yellow
$ConfigPath = Join-Path $PSScriptRoot "..\config.json"
if (Test-Path $ConfigPath) {
    try {
        $json = Get-Content $ConfigPath -Raw | ConvertFrom-Json
        
        # Optimize for Real-Time
        $json.inference_interval = 1 # Process every frame
        $json.alert_cooldown = 3000   # Lower cooldown for snappier alerts
        
        # Enable Hardware Acceleration hints (if we had them in config)
        
        $json | ConvertTo-Json -Depth 10 | Set-Content $ConfigPath
        Write-Host "  [OK] Configuration tuned for low latency." -ForegroundColor Green
    } catch {
        Write-Host "  [ERROR] Failed to update config.json" -ForegroundColor Red
    }
} else {
    Write-Host "  [WARN] config.json not found." -ForegroundColor Yellow
}

# 4. Engine Pre-Warming (The "Google Engineer" Fix)
Write-Host ""
Write-Host "[4/4] Pre-building TensorRT Engine (Warm-up)..." -ForegroundColor Yellow
Write-Host "  This ensures the system starts instantly next time." -ForegroundColor Gray
Write-Host "  Please wait, this may take a few minutes..." -ForegroundColor Gray

# We need to run this inside the container because the engine must match the container's TensorRT version.
if (Get-Command docker-compose -ErrorAction SilentlyContinue) {
    # Ensure config and model are available
    # We use 'run --rm' to spin up a temporary container, map the volumes, and run the build command
    try {
        # Note: We rely on the docker-compose.yml 'engine' service definition for volumes/env
        # Overriding entrypoint to run the build flag
        docker-compose run --rm --entrypoint "./build/main_app --build-engine-only" engine
        
        if ($LASTEXITCODE -eq 0) {
            Write-Host "  [SUCCESS] Engine built and verified!" -ForegroundColor Green
        } else {
            Write-Host "  [ERROR] Engine build failed. Check logs above." -ForegroundColor Red
        }
    } catch {
        Write-Host "  [ERROR] Docker command failed: $_" -ForegroundColor Red
    }
} else {
    Write-Host "  [ERROR] Docker Compose not found. Skipping engine pre-build." -ForegroundColor Red
}

Write-Host ""
Write-Host "==============================================" -ForegroundColor Cyan
Write-Host "   OPTIMIZATION COMPLETE" -ForegroundColor Cyan
Write-Host "   You can now run 'tools\\start_system.bat'" -ForegroundColor Cyan     
Write-Host "==============================================" -ForegroundColor Cyan
