@echo off
rem Build SearchClient_asio from the command line without starting the IDE.
rem The first argument is Debug or Release (Debug by default).

setlocal EnableExtensions DisableDelayedExpansion
set "BUILD_RC=0"
set "CFG=%~1"
set "MODE=%~2"
set "RSVARS=C:\Program Files (x86)\Embarcadero\Studio\22.0\bin\rsvars.bat"

if "%CFG%"=="" set "CFG=Debug"
if /I "%CFG%"=="Debug" goto :debug
if /I "%CFG%"=="Release" goto :release
echo ERROR: Configuration must be Debug or Release.
set "BUILD_RC=2"
goto :finish

:release
if not "%MODE%"=="" (
    echo ERROR: Release NoLink and additional modes are not supported.
    set "BUILD_RC=2"
    goto :finish
)
call "%~dp0release.bat"
set "BUILD_RC=%ERRORLEVEL%"
goto :finish

:debug
if "%MODE%"=="" goto :debug_mode_ok
if /I "%MODE%"=="NoLink" (
    set "EXTRA=/p:_SkipLink=true"
    goto :debug_mode_ok
)
echo ERROR: Second argument must be NoLink.
set "BUILD_RC=2"
goto :finish

:debug_mode_ok
if not exist "%RSVARS%" (
    echo ERROR: rsvars.bat not found: "%RSVARS%"
    set "BUILD_RC=9009"
    goto :finish
)
call "%RSVARS%"
set "BUILD_RC=%ERRORLEVEL%"
if not "%BUILD_RC%"=="0" goto :finish
pushd "%~dp0.."
set "BUILD_RC=%ERRORLEVEL%"
if not "%BUILD_RC%"=="0" goto :finish
msbuild SearchClient_asio.cbproj /t:Build /p:Config=Debug /p:Platform=Win32 %EXTRA% /v:n /nologo
set "BUILD_RC=%ERRORLEVEL%"
popd

:finish
endlocal & exit /b %BUILD_RC%
