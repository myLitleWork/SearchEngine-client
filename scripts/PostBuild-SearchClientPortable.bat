@echo off
setlocal EnableExtensions DisableDelayedExpansion
set "BUILD_RC=0"
set "SCRIPT_DIR=%~dp0"
set "BUILD_DIR=%SCRIPT_DIR%..\Win32\Release"

set "POWERSHELL=%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe"
if not exist "%POWERSHELL%" (
  echo ERROR: Windows PowerShell not found: "%POWERSHELL%"
  set "BUILD_RC=9009"
  goto :finish
)

"%POWERSHELL%" -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%PostBuild-SearchClientPortable.ps1" -BuildDirectory "%BUILD_DIR%"
set "BUILD_RC=%ERRORLEVEL%"

:finish
endlocal & exit /b %BUILD_RC%
