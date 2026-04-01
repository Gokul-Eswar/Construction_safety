@echo off
title Sentinel Safety System Installer

set "SCRIPT_DIR=%~dp0"
if exist "%SCRIPT_DIR%..\installer\setup.ps1" (
	cd /d "%SCRIPT_DIR%.."
) else (
	cd /d "%SCRIPT_DIR%"
)

powershell -NoProfile -ExecutionPolicy Bypass -File "installer\setup.ps1"
exit
