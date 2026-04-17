param(
    [int]$DurationSec = 60,
    [string]$OutputDir = "docs"
)

$ErrorActionPreference = "Stop"

function Write-Info($msg) {
    Write-Host "[INFO] $msg" -ForegroundColor Cyan
}

function Write-WarnMsg($msg) {
    Write-Host "[WARN] $msg" -ForegroundColor Yellow
}

function Ensure-Dir($path) {
    if (-not (Test-Path $path)) {
        New-Item -Path $path -ItemType Directory | Out-Null
    }
}

Ensure-Dir $OutputDir

$dbOut = Join-Path $OutputDir "metrics_db_snapshot.json"
$rtOut = Join-Path $OutputDir "metrics_runtime_snapshot.json"

Write-Info "Capturing persisted database metrics -> $dbOut"
node .\tools\collect_db_metrics.js > $dbOut

Write-Info "Checking Docker daemon availability"
$dockerOk = $true
try {
    docker info | Out-Null
} catch {
    $dockerOk = $false
}

if (-not $dockerOk) {
    Write-WarnMsg "Docker daemon is not reachable. Runtime telemetry capture will likely be empty unless another MQTT broker is already running on localhost:1883."
} else {
    Write-Info "Starting MQTT broker container"
    docker compose up -d mqtt | Out-Null
}

Write-Info "Capturing runtime MQTT telemetry for $DurationSec seconds -> $rtOut"
$env:DURATION_SEC = "$DurationSec"
node .\tools\collect_runtime_metrics.js > $rtOut
Remove-Item Env:DURATION_SEC -ErrorAction SilentlyContinue

Write-Info "Done. Files generated:"
Write-Host " - $dbOut"
Write-Host " - $rtOut"

Write-Info "Quick summary from runtime snapshot:"
$rt = Get-Content $rtOut -Raw | ConvertFrom-Json
Write-Host " telemetry count : $($rt.counts.telemetry)"
Write-Host " heartbeat count : $($rt.counts.heartbeat)"
Write-Host " violations count: $($rt.counts.violations)"
if ($rt.counts.telemetry -eq 0) {
    Write-WarnMsg "No telemetry received. Start the system using tools/Sentinel.bat option 1, then rerun this script."
}
