@echo off
if /I "%~1"=="/quiet" (
  powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Verify-Package.ps1" -Quiet
) else (
  powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Verify-Package.ps1"
)
exit /b %errorlevel%
