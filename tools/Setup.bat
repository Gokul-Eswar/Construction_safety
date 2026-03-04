@echo off
title Sentinel Safety System Installer
cd /d "%~dp0"
powershell -NoProfile -ExecutionPolicy Bypass -File "installer\setup.ps1"
exit
