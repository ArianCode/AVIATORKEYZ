@echo off
REM =============================================================================
REM  AviatorKeyz — Run Standalone (Windows)
REM  Usage:
REM    run_standalone_windows.bat
REM =============================================================================

setlocal

set APP_PATH=build\AviatorKeyz_artefacts\Release\Standalone\AviatorKeyz.exe

if not exist "%APP_PATH%" (
    echo Standalone app not found at:
    echo   %APP_PATH%
    echo.
    echo Build it first:
    echo   .\scripts\build_windows.bat
    exit /b 1
)

echo Launching standalone:
echo   %APP_PATH%
start "" "%APP_PATH%"

endlocal
