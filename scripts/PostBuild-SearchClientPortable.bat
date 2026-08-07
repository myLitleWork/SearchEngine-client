@echo off
setlocal
set "SCRIPT_DIR=%~dp0"
set "BUILD_DIR=%~1"
if "%BUILD_DIR%"=="" set "BUILD_DIR=%SCRIPT_DIR%..\Win32\Release"

set "POWERSHELL=%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe"
if not exist "%POWERSHELL%" (
  echo ERROR: Windows PowerShell not found: "%POWERSHELL%"
  exit /b 9009
)

"%POWERSHELL%" -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%PostBuild-SearchClientPortable.ps1" -BuildDirectory "%BUILD_DIR%"
exit /b %ERRORLEVEL%
