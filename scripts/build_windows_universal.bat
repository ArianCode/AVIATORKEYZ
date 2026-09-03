@echo off
REM =============================================================================
REM  Aviation - Windows universal VST3 build (x64 + x86 in ONE bundle)
REM
REM  Produces a single Aviation.vst3 bundle containing both:
REM    Contents\x86_64-win\Aviation.vst3   -> all 64-bit FL hosts (11.1+, 20, 21+, 25)
REM    Contents\x86-win\Aviation.vst3      -> FL 11 32-bit
REM
REM  One x64 binary already covers every Intel AND AMD CPU, and both Windows 10
REM  and 11 - those are not separate builds. Host bitness is the only real split.
REM
REM  Usage:
REM    scripts\build_windows_universal.bat            (x64 + x86)
REM    scripts\build_windows_universal.bat x64only    (skip the 32-bit half)
REM    scripts\build_windows_universal.bat clean
REM
REM  Requires: Visual Studio 2022 with both x64 and x86 toolchains installed.
REM =============================================================================

setlocal EnableDelayedExpansion

set ROOT=%~dp0..
cd /d "%ROOT%"

set BUILD_X64=build-win-x64
set BUILD_X86=build-win-x86
set CONFIG=Release
set WANT_X86=1

if /I "%1"=="x64only" set WANT_X86=0
if /I "%1"=="clean" (
    echo Cleaning %BUILD_X64% and %BUILD_X86%...
    if exist %BUILD_X64% rd /s /q %BUILD_X64%
    if exist %BUILD_X86% rd /s /q %BUILD_X86%
    echo Done.
    exit /b 0
)

echo === Aviation Windows universal build (%CONFIG%) ===
echo.

REM --- Build identity ---------------------------------------------------------
REM BuildInfo.h is captured at CONFIGURE time. Both trees are configured fresh
REM below, so each half is stamped with the SHA that is HEAD right now.
for /f "delims=" %%i in ('git rev-parse --short^=10 HEAD 2^>nul') do set GIT_SHA=%%i
for /f "delims=" %%i in ('git describe --tags --always --dirty 2^>nul') do set GIT_DESC=%%i
if not defined GIT_SHA set GIT_SHA=unknown
if not defined GIT_DESC set GIT_DESC=untagged
echo Commit:   %GIT_SHA%
echo Describe: %GIT_DESC%
echo %GIT_DESC% | findstr /C:"dirty" >nul
if %errorlevel% equ 0 (
    echo.
    echo WARNING: working tree is dirty - this binary is NOT reproducible from a SHA.
    echo          Do not ship it as an RC. Commit and re-run.
    echo.
)

REM --- x64 --------------------------------------------------------------------
echo [1/4] Configuring + building x64...
cmake -S . -B %BUILD_X64% -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=%CONFIG%
if %errorlevel% neq 0 ( echo x64 configure failed. & exit /b 1 )
cmake --build %BUILD_X64% --config %CONFIG% --parallel
if %errorlevel% neq 0 ( echo x64 build failed. & exit /b 1 )

set BUNDLE=%BUILD_X64%\Aviation_artefacts\%CONFIG%\VST3\Aviation.vst3
if not exist "%BUNDLE%" ( echo ERROR: x64 bundle not found at %BUNDLE% & exit /b 1 )
if not exist "%BUNDLE%\Contents\x86_64-win\Aviation.vst3" (
    echo ERROR: x64 inner DLL missing - bundle layout is not what packaging expects.
    exit /b 1
)

if %WANT_X86%==0 (
    echo.
    echo [2/4] x86 SKIPPED ^(x64only^).
    echo [3/4] merge SKIPPED.
    goto verify
)

REM --- x86 --------------------------------------------------------------------
echo.
echo [2/4] Configuring + building x86 ^(Win32^)...
cmake -S . -B %BUILD_X86% -G "Visual Studio 17 2022" -A Win32 -DCMAKE_BUILD_TYPE=%CONFIG%
if %errorlevel% neq 0 ( echo x86 configure failed. & exit /b 1 )
cmake --build %BUILD_X86% --config %CONFIG% --parallel
if %errorlevel% neq 0 ( echo x86 build failed. & exit /b 1 )

set X86_DLL=%BUILD_X86%\Aviation_artefacts\%CONFIG%\VST3\Aviation.vst3\Contents\x86-win\Aviation.vst3
if not exist "%X86_DLL%" ( echo ERROR: x86 inner DLL not found at %X86_DLL% & exit /b 1 )

REM --- Merge ------------------------------------------------------------------
REM A VST3 bundle may carry several architecture folders side by side; each host
REM loads the one matching its own bitness. Merging into the x64 bundle keeps a
REM single artifact for every FL version.
echo.
echo [3/4] Merging x86 into the x64 bundle...
if not exist "%BUNDLE%\Contents\x86-win" mkdir "%BUNDLE%\Contents\x86-win"
copy /Y "%X86_DLL%" "%BUNDLE%\Contents\x86-win\Aviation.vst3" >nul
if %errorlevel% neq 0 ( echo ERROR: merge copy failed. & exit /b 1 )

:verify
echo.
echo [4/4] Verifying bundle...
set FAIL=0
if not exist "%BUNDLE%\Contents\x86_64-win\Aviation.vst3" ( echo MISSING: x86_64-win & set FAIL=1 )
if %WANT_X86%==1 (
    if not exist "%BUNDLE%\Contents\x86-win\Aviation.vst3" ( echo MISSING: x86-win & set FAIL=1 )
)
if !FAIL! neq 0 ( echo BUNDLE VERIFY FAILED. & exit /b 1 )

echo.
echo ===================================================
echo  BUILD SUCCEEDED
echo  Bundle: %BUNDLE%
echo ===================================================
dir /b "%BUNDLE%\Contents"
echo.
echo  Static MSVC runtime - no VC++ Redistributable needed on the client machine.
echo.
echo  Install for FL testing - copy the SAME bundle to BOTH paths:
echo    C:\Program Files\Common Files\VST3\          ^(64-bit FL: 11.1+, 20, 21+, 25^)
echo    C:\Program Files ^(x86^)\Common Files\VST3\    ^(FL 11 32-bit^)
echo.
echo  Then: Options ^> Manage plugins ^> Find more plugins ^> Start scan
echo.
echo  Next: scripts\package_prototype_windows.bat

endlocal
