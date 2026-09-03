@echo off
REM =============================================================================
REM  Aviation — Prototype 1 RC1 Windows installer build
REM
REM  Prerequisites:
REM    1. scripts\build_windows.bat            (x64 Release VST3)
REM    2. scripts\package_prototype_windows.bat (ZIP + VERSION.txt + SHA256)
REM    3. Inno Setup 6.3+   ->  winget install JRSoftware.InnoSetup
REM                         or  choco install innosetup
REM
REM  Usage:
REM    scripts\build_installer_windows.bat
REM
REM  Output:
REM    dist\Aviation_Prototype1_RC1_Windows_x64_Setup.exe
REM    dist\Aviation_Prototype1_RC1_Windows_x64_Setup.exe.sha256
REM
REM  Optional signing (leave unset to produce an unsigned installer):
REM    set SIGN_PFX=C:\path\to\cert.pfx
REM    set SIGN_PFX_PASSWORD=...
REM    set SIGN_TIMESTAMP_URL=http://timestamp.digicert.com
REM =============================================================================

setlocal EnableDelayedExpansion

set ROOT=%~dp0..
cd /d "%ROOT%"

set BUILD_DIR=build
set CONFIG=Release
set VST3_SRC=%ROOT%\%BUILD_DIR%\Aviation_artefacts\%CONFIG%\VST3\Aviation.vst3
set DOCS_SRC=%ROOT%\release\prototype1
set OUT_DIR=%ROOT%\dist
set SETUP_EXE=%OUT_DIR%\Aviation_Prototype1_RC1_Windows_x64_Setup.exe

echo === Aviation Prototype 1 RC1 — Windows installer ===

REM --- Refuse to build an installer from an unreproducible tree ---------------
for /f "delims=" %%i in ('git describe --tags --always --dirty 2^>nul') do set GIT_DESC=%%i
for /f "delims=" %%i in ('git rev-parse --short^=10 HEAD 2^>nul') do set GIT_SHA=%%i
if not defined GIT_SHA set GIT_SHA=unknown
if not defined GIT_DESC set GIT_DESC=untagged
echo !GIT_DESC! | findstr /C:"dirty" >nul
if !errorlevel! equ 0 (
    echo ERROR: working tree is dirty. Commit and rebuild before packaging an RC.
    exit /b 1
)
echo Commit identity: !GIT_DESC!  ^(!GIT_SHA!^)

if not exist "%VST3_SRC%" (
    echo ERROR: Release VST3 not found at:
    echo   %VST3_SRC%
    echo Run scripts\build_windows.bat first.
    exit /b 1
)

REM --- Locate Inno Setup ------------------------------------------------------
set ISCC=
for %%P in (
    "%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe"
    "%ProgramFiles%\Inno Setup 6\ISCC.exe"
) do (
    if exist %%P set ISCC=%%P
)
if not defined ISCC (
    where ISCC.exe >nul 2>&1
    if !errorlevel! equ 0 set ISCC=ISCC.exe
)
if not defined ISCC (
    echo ERROR: Inno Setup ^(ISCC.exe^) not found.
    echo   winget install JRSoftware.InnoSetup
    exit /b 1
)
echo Inno Setup: !ISCC!

if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"

REM --- Compile ----------------------------------------------------------------
echo.
echo Compiling installer ^(LZMA2 over ~250 MB — this takes several minutes^)...
!ISCC! ^
    /DAppVersion=0.1.0-rc1 ^
    /DGitSha=%GIT_SHA% ^
    /DVst3Src="%VST3_SRC%" ^
    /DDocsSrc="%DOCS_SRC%" ^
    /DOutDir="%OUT_DIR%" ^
    "%ROOT%\scripts\installer_windows.iss"
if !errorlevel! neq 0 (
    echo Installer compilation failed.
    exit /b 1
)

if not exist "%SETUP_EXE%" (
    echo ERROR: expected installer not produced: %SETUP_EXE%
    exit /b 1
)

REM --- Optional Authenticode signing -----------------------------------------
if defined SIGN_PFX (
    if not defined SIGN_TIMESTAMP_URL set SIGN_TIMESTAMP_URL=http://timestamp.digicert.com
    echo.
    echo Signing installer...
    signtool sign /f "%SIGN_PFX%" /p "%SIGN_PFX_PASSWORD%" ^
        /fd SHA256 /tr "%SIGN_TIMESTAMP_URL%" /td SHA256 "%SETUP_EXE%"
    if !errorlevel! neq 0 (
        echo ERROR: signing failed.
        exit /b 1
    )
    signtool verify /pa /v "%SETUP_EXE%"
) else (
    echo.
    echo GATE OPEN: installer is UNSIGNED — Windows SmartScreen will warn the tester.
    echo            Set SIGN_PFX / SIGN_PFX_PASSWORD to sign it.
)

REM --- Checksum LAST, after the artifact is final -----------------------------
echo.
echo == SHA-256 ==
certutil -hashfile "%SETUP_EXE%" SHA256 | findstr /V ":" > "%SETUP_EXE%.sha256"
type "%SETUP_EXE%.sha256"

echo.
echo ===================================================
echo  INSTALLER COMPLETE
echo  %SETUP_EXE%
echo ===================================================
echo.
echo Next:
echo   1. VST3 validator on build\Aviation_artefacts\Release\VST3\Aviation.vst3
echo   2. Install on a CLEAN Windows machine ^(no VS, no source, no CMake^)
echo   3. FL matrix — FL25 first, then FL21+, FL20, FL11 ^(docs\PROTOTYPE1_FL_MATRIX.md^)
echo   Do not modify the .exe after the SHA-256 above is recorded.

endlocal
