@echo off
REM =============================================================================
REM  AviatorKeyz — Windows Build Script (secondary platform; macOS: build_macos.sh)
REM  Usage:
REM    build_windows.bat          (Release build)
REM    build_windows.bat debug    (Debug build)
REM    build_windows.bat clean    (Delete build directory)
REM =============================================================================

setlocal

set BUILD_DIR=build
set CONFIG=Release

if "%1"=="debug" set CONFIG=Debug
if "%1"=="Debug" set CONFIG=Debug
if "%1"=="clean" (
    echo Cleaning build directory...
    if exist %BUILD_DIR% rd /s /q %BUILD_DIR%
    echo Done.
    exit /b 0
)

echo === AviatorKeyz Build (%CONFIG%) ===
echo.

REM --- Configure ---
echo [1/3] Configuring CMake...
cmake -S . -B %BUILD_DIR% -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_BUILD_TYPE=%CONFIG%
if %errorlevel% neq 0 (
    echo CMake configuration failed.
    exit /b 1
)

REM --- Build ---
echo.
echo [2/3] Building...
cmake --build %BUILD_DIR% --config %CONFIG% --parallel
if %errorlevel% neq 0 (
    echo Build failed.
    exit /b 1
)

REM --- Locate output ---
echo.
echo [3/3] Locating VST3...
set VST3_PATH=%BUILD_DIR%\AviatorKeyz_artefacts\%CONFIG%\VST3\AviatorKeyz.vst3
if exist "%VST3_PATH%" (
    echo.
    echo ===================================================
    echo  BUILD SUCCEEDED
    echo  VST3: %VST3_PATH%
    echo ===================================================
    echo.
    echo To install for FL Studio testing:
    echo   Copy "%VST3_PATH%" to:
    echo   C:\Program Files\Common Files\VST3\
    echo.
    echo Then rescan plugins in FL Studio:
    echo   Options ^> Manage plugins ^> Find more plugins ^> Start scan
) else (
    echo WARNING: Could not find VST3 at expected path.
    echo Check build output above for actual artifact location.
)

endlocal
