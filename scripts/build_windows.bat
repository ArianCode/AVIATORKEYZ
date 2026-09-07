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

REM Export AVIATORKEYZ_ENABLE_LTO=OFF to skip link-time optimisation when the MSVC
REM link stage stalls or runs out of memory over the embedded factory bank.
if not defined AVIATORKEYZ_ENABLE_LTO set AVIATORKEYZ_ENABLE_LTO=ON

REM Export AVIATORKEYZ_CMAKE_GENERATOR to build with a Visual Studio other than
REM 2022 (the hosted CI runner image ships Visual Studio 2026).
if not defined AVIATORKEYZ_CMAKE_GENERATOR set AVIATORKEYZ_CMAKE_GENERATOR=Visual Studio 17 2022

if "%1"=="debug" set CONFIG=Debug
if "%1"=="Debug" set CONFIG=Debug
if "%1"=="clean" (
    echo Cleaning build directory...
    if exist %BUILD_DIR% rd /s /q %BUILD_DIR%
    echo Done.
    exit /b 0
)

echo === Aviation Build (%CONFIG%, x64) ===
echo.

REM --- Release identity guard -------------------------------------------------
REM BuildInfo.h is regenerated on every configure below, so the SHA stamped into
REM the binary is whatever HEAD is right now. A dirty tree yields "<sha>-dirty".
for /f "delims=" %%i in ('git rev-parse --short^=10 HEAD 2^>nul') do set GIT_SHA=%%i
for /f "delims=" %%i in ('git describe --tags --always --dirty 2^>nul') do set GIT_DESC=%%i
if not defined GIT_SHA set GIT_SHA=unknown
if not defined GIT_DESC set GIT_DESC=untagged
echo Commit:   %GIT_SHA%
echo Describe: %GIT_DESC%
echo %GIT_DESC% | findstr /C:"dirty" >nul
if %errorlevel% equ 0 (
    echo.
    echo WARNING: working tree is dirty — this binary is NOT reproducible from a SHA.
    echo          Do not ship it as an RC. Commit and re-run.
    echo.
)

REM --- Configure ---
echo [1/3] Configuring CMake...
cmake -S . -B %BUILD_DIR% -G "%AVIATORKEYZ_CMAKE_GENERATOR%" -A x64 ^
    -DCMAKE_BUILD_TYPE=%CONFIG% ^
    -DAVIATORKEYZ_ENABLE_LTO=%AVIATORKEYZ_ENABLE_LTO%
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
set VST3_PATH=%BUILD_DIR%\Aviation_artefacts\%CONFIG%\VST3\Aviation.vst3
if exist "%VST3_PATH%" (
    echo.
    echo ===================================================
    echo  BUILD SUCCEEDED
    echo  VST3: %VST3_PATH%
    echo ===================================================
    echo.
    echo  Static MSVC runtime — no VC++ Redistributable needed on the client machine.
    echo  ^(Confirm with: dumpbin /DEPENDENTS "%VST3_PATH%\Contents\x86_64-win\Aviation.vst3"^)
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
