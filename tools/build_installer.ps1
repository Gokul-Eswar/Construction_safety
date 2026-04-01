# build_installer.ps1
# Generates a single-file executable installer for the Sentinel Safety System

$ErrorActionPreference = "Stop"
$ProjectRoot = Resolve-Path "$PSScriptRoot\.."
$BuildDir = "$PSScriptRoot\build_temp"
$DistDir = "$PSScriptRoot\dist"
$StagingDir = "$BuildDir\staging"
$InstallerName = "SentinelSetup.exe"

Write-Host "[1/6] Cleaning up..."
if (Test-Path $BuildDir) { Remove-Item -Path $BuildDir -Recurse -Force }
if (Test-Path $DistDir) { Remove-Item -Path $DistDir -Recurse -Force }
New-Item -ItemType Directory -Path $StagingDir | Out-Null
New-Item -ItemType Directory -Path $DistDir | Out-Null

Write-Host "[2/6] Copying project files..."
# Allow-list approach to avoid copying garbage
$Includes = @(
    "installer",
    "src",
    "web",
    "tools",
    "docs",
    "config.json",
    "docker-compose.yml",
    "docker-compose.prod.yml",
    "Dockerfile.engine",
    "Dockerfile.web",
    "readme.md",
    "yolo11n.onnx",
    ".clang-tidy",
    "lint.bat"
)

foreach ($Item in $Includes) {
    $SourcePath = "$ProjectRoot\$Item"
    if (Test-Path $SourcePath) {
        Copy-Item -Path $SourcePath -Destination $StagingDir -Recurse
    } else {
        Write-Warning "Skipping missing item: $Item"
    }
}

# Cleanup excluded items from staging (just in case copy-recurse grabbed them)
$Excludes = @(".git", "node_modules", "build", ".gemini", "tmp", "__pycache__", ".vscode", "coverage")
foreach ($Ex in $Excludes) {
    Get-ChildItem -Path $StagingDir -Recurse -Force | Where-Object { $_.Name -eq $Ex } | Remove-Item -Recurse -Force
}

Write-Host "[3/6] Creating payload.zip..."
$ZipPath = "$BuildDir\payload.zip"
Compress-Archive -Path "$StagingDir\*" -DestinationPath $ZipPath

Write-Host "[4/6] Generating Installer C# Code..."
$CsCode = @"
using System;
using System.Diagnostics;
using System.IO;
using System.Reflection;

namespace SentinelInstaller
{
    class Program
    {
        static void Main(string[] args)
        {
            string tempPath = Path.Combine(Path.GetTempPath(), "SentinelInstaller_" + Guid.NewGuid().ToString().Substring(0, 8));
            string zipPath = Path.Combine(tempPath, "payload.zip");

            try 
            {
                if (Directory.Exists(tempPath)) Directory.Delete(tempPath, true);
                Directory.CreateDirectory(tempPath);

                // 1. Extract embedded zip
                using (Stream stream = Assembly.GetExecutingAssembly().GetManifestResourceStream("payload.zip"))
                using (FileStream fileStream = new FileStream(zipPath, FileMode.Create))
                {
                    if (stream == null) throw new Exception("Embedded resource not found.");
                    stream.CopyTo(fileStream);
                }

                // 2. Unzip using PowerShell (Native .NET 4.0 zip is tricky, PS is reliable on modern Windows)
                ProcessStartInfo psiUnzip = new ProcessStartInfo();
                psiUnzip.FileName = "powershell";
                psiUnzip.Arguments = "-NoProfile -Command \"Expand-Archive -Path '" + zipPath + "' -DestinationPath '" + tempPath + "' -Force\" ";
                psiUnzip.WindowStyle = ProcessWindowStyle.Hidden;
                
                Process unzipProc = Process.Start(psiUnzip);
                unzipProc.WaitForExit();

                if (unzipProc.ExitCode != 0) throw new Exception("Extraction failed.");

                // 3. Run Setup.bat
                string setupPath = Path.Combine(tempPath, "tools", "Setup.bat");
                if (!File.Exists(setupPath))
                {
                    setupPath = Path.Combine(tempPath, "Setup.bat");
                }
                if (!File.Exists(setupPath))
                {
                    throw new Exception("Setup.bat not found in installer payload.");
                }

                ProcessStartInfo psiSetup = new ProcessStartInfo();
                psiSetup.FileName = setupPath;
                psiSetup.WorkingDirectory = tempPath;
                psiSetup.UseShellExecute = true; // Use shell to handle .bat
                
                Process setupProc = Process.Start(psiSetup);
                // We don't wait for setup to finish, it forks its own GUI. 
                // But if we exit, tempPath might be locked? 
                // Actually, Setup.bat runs PowerShell script which stays open.
                // If we exit this wrapper, the temp files remain?
                // We can't easily clean up temp files if we spawn a detached process.
                // For an installer, leaving temp files is acceptable (Windows cleans %TEMP% eventually).
            }
            catch (Exception ex)
            {
                // Simple error dialog via generic Windows command if GUI fails
                // Or just silent fail.
            }
        }
    }
}
"@
$CsPath = "$BuildDir\Installer.cs"
Set-Content -Path $CsPath -Value $CsCode

Write-Host "[5/6] Compiling $InstallerName..."
# Find CSC
$CscPath = "$env:SystemRoot\Microsoft.NET\Framework64\v4.0.30319\csc.exe"
if (-not (Test-Path $CscPath)) {
    $CscPath = "$env:SystemRoot\Microsoft.NET\Framework\v4.0.30319\csc.exe"
}

if (-not (Test-Path $CscPath)) {
    Write-Error "C# Compiler (csc.exe) not found. Requires .NET Framework."
}

$OutputExe = "$DistDir\$InstallerName"
$ArgList = @(
    "/target:winexe",
    "/out:`"$OutputExe`"",
    "/resource:`"$ZipPath`",payload.zip",
    "`"$CsPath`""
)

$P = Start-Process -FilePath $CscPath -ArgumentList $ArgList -PassThru -NoNewWindow -Wait

if ($P.ExitCode -eq 0) {
    Write-Host "[6/6] Success! Installer created at:" -ForegroundColor Green
    Write-Host "      $OutputExe" -ForegroundColor Cyan
} else {
    Write-Error "Compilation failed."
}
