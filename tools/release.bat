@echo off
rem Canonical agent entrypoint for the full SearchClient Release workflow.

setlocal EnableExtensions DisableDelayedExpansion
set "BUILD_RC=0"
set "SCRIPT_DIR=%~dp0"
set "POWERSHELL=%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe"

if not exist "%POWERSHELL%" (
    echo ERROR: Windows PowerShell not found: "%POWERSHELL%"
    set "BUILD_RC=9009"
    goto :finish
)

"%POWERSHELL%" -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%..\scripts\Build-SearchClientPackage.ps1" -AllowDirty %*
rem Capture PowerShell's result immediately; it is the Release workflow result.
set "BUILD_RC=%ERRORLEVEL%"

:finish
endlocal & exit /b %BUILD_RC%
