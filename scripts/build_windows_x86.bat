@echo off
REM =============================================================================
REM  Aviation — Windows x86 VST3 build (Prototype 1)
REM  Use ONLY if FL Studio 11 32-bit is confirmed (see docs/PROTOTYPE1_TARGET_ENV.md)
REM =============================================================================

setlocal
set BUILD_DIR=build-x86
set CONFIG=Release

if "%1"=="clean" (
    echo Cleaning %BUILD_DIR%...
    if exist %BUILD_DIR% rd /s /q %BUILD_DIR%
    echo Done.
    exit /b 0
)

echo === Aviation x86 Build (%CONFIG%) ===
echo.

echo [1/3] Configuring CMake (Win32)...
cmake -S . -B %BUILD_DIR% -G "Visual Studio 17 2022" -A Win32 -DCMAKE_BUILD_TYPE=%CONFIG%
if %errorlevel% neq 0 exit /b 1

echo.
echo [2/3] Building...
cmake --build %BUILD_DIR% --config %CONFIG% --parallel
if %errorlevel% neq 0 exit /b 1

echo.
echo [3/3] Locating VST3...
set VST3_PATH=%BUILD_DIR%\Aviation_artefacts\%CONFIG%\VST3\Aviation.vst3
if exist "%VST3_PATH%" (
    echo BUILD SUCCEEDED: %VST3_PATH%
    echo Inner DLL: %VST3_PATH%\Contents\x86-win\Aviation.vst3
    echo Install to: C:\Program Files ^(x86^)\Common Files\VST3\
) else (
    echo WARNING: VST3 not found at expected path.
    exit /b 1
)

endlocal
