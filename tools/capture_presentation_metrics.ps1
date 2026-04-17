param(
    [int]$DurationSec = 60,
    [string]$OutputDir = "docs",
    [string]$ResultsCsv = "",
    [string]$ConfusionPath = ""
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
$graphsDir = Join-Path $OutputDir "graphs"
$reportOut = Join-Path $graphsDir "report.html"

Write-Info "Capturing persisted database metrics -> $dbOut"
node .\tools\collect_db_metrics.js | Out-File -FilePath $dbOut -Encoding utf8

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
node .\tools\collect_runtime_metrics.js | Out-File -FilePath $rtOut -Encoding utf8
Remove-Item Env:DURATION_SEC -ErrorAction SilentlyContinue

Write-Info "Generating presentation graphs and report"
$graphArgs = @(
    ".\tools\generate_report_graphs.py"
    "--runtime", $rtOut
    "--db", $dbOut
)
if ($ResultsCsv -and (Test-Path $ResultsCsv)) {
    $graphArgs += @("--results", $ResultsCsv)
} elseif ($ResultsCsv) {
    Write-WarnMsg "Results CSV not found at '$ResultsCsv'. Skipping training curves."
}
if ($ConfusionPath -and (Test-Path $ConfusionPath)) {
    $graphArgs += @("--confusion", $ConfusionPath)
} elseif ($ConfusionPath) {
    Write-WarnMsg "Confusion input not found at '$ConfusionPath'. Skipping confusion matrix graph."
}

& c:/msys64/ucrt64/bin/python.exe @graphArgs

Write-Info "Done. Files generated:"
Write-Host " - $dbOut"
Write-Host " - $rtOut"
Write-Host " - $reportOut"

Write-Info "Quick summary from runtime snapshot:"
$rt = Get-Content $rtOut -Raw | ConvertFrom-Json
Write-Host " telemetry count : $($rt.counts.telemetry)"
Write-Host " heartbeat count : $($rt.counts.heartbeat)"
Write-Host " violations count: $($rt.counts.violations)"
if ($rt.counts.telemetry -eq 0) {
    Write-WarnMsg "No telemetry received. Start the system using tools/Sentinel.bat option 1, then rerun this script."
}
