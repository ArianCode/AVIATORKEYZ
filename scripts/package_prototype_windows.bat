@echo off
REM =============================================================================
REM  Aviation — Prototype 1 RC1 Windows packaging
REM
REM  Prerequisites:
REM    scripts\build_windows.bat          (x64 Release)
REM    scripts\build_windows_x86.bat      (optional, FL11 32-bit only)
REM
REM  Usage:
REM    scripts\package_prototype_windows.bat
REM    scripts\package_prototype_windows.bat --skip-dumpbin
REM =============================================================================

setlocal EnableDelayedExpansion

set ROOT=%~dp0..
cd /d "%ROOT%"

set BUILD_DIR=build
set CONFIG=Release
set OUT_DIR=dist\Aviation_Prototype1_RC1_Windows
set ZIP_NAME=Aviation_Prototype1_RC1_Windows_x64.zip
set VST3_SRC=%BUILD_DIR%\Aviation_artefacts\%CONFIG%\VST3\Aviation.vst3
set INNER_DLL=%VST3_SRC%\Contents\x86_64-win\Aviation.vst3
set SKIP_DUMPBIN=0

:parse_args
if "%~1"=="" goto done_args
if /I "%~1"=="--skip-dumpbin" set SKIP_DUMPBIN=1
shift
goto parse_args
:done_args

if not exist "%VST3_SRC%" (
    echo ERROR: Release VST3 not found. Run: scripts\build_windows.bat
    exit /b 1
)

echo === Prototype 1 RC1 packaging ===

for /f "delims=" %%i in ('git describe --tags --always --dirty 2^>nul') do set GIT_DESC_PRE=%%i
echo !GIT_DESC_PRE! | findstr /C:"dirty" >nul
if !errorlevel! equ 0 (
    echo ERROR: working tree is dirty. The recorded SHA would not rebuild this binary.
    echo        Commit, rebuild, and re-run before packaging an RC.
    exit /b 1
)
echo Commit identity: !GIT_DESC_PRE!

if exist dist rmdir /s /q dist
mkdir "%OUT_DIR%\VST3"
mkdir "%OUT_DIR%\TEST"
mkdir "%OUT_DIR%\DOCS"

echo Copying VST3 bundle...
xcopy /E /I /Y "%VST3_SRC%" "%OUT_DIR%\VST3\Aviation.vst3" >nul

echo Copying release docs...
copy /Y release\prototype1\README_FIRST.txt "%OUT_DIR%\" >nul
copy /Y release\prototype1\DOCS\INSTALL_WINDOWS.txt "%OUT_DIR%\DOCS\" >nul
copy /Y release\prototype1\DOCS\KNOWN_ISSUES.txt "%OUT_DIR%\DOCS\" >nul
copy /Y release\prototype1\DOCS\FEEDBACK_TEMPLATE.txt "%OUT_DIR%\DOCS\" >nul
copy /Y release\prototype1\TEST\TEST_CHECKLIST.txt "%OUT_DIR%\TEST\" >nul
if exist release\prototype1\TEST\Prototype_Test.mid (
    copy /Y release\prototype1\TEST\Prototype_Test.mid "%OUT_DIR%\TEST\" >nul
)

REM --- Runtime dependencies ---
set RUNTIME_DEPS=see dumpbin output below
if %SKIP_DUMPBIN%==0 (
    where dumpbin >nul 2>&1
    if !errorlevel! equ 0 (
        echo.
        echo == dumpbin /DEPENDENTS ==
        dumpbin /DEPENDENTS "%INNER_DLL%" > dist\dumpbin_dependents.txt
        type dist\dumpbin_dependents.txt
        set RUNTIME_DEPS=recorded in dist\dumpbin_dependents.txt (static CRT expected - no VCRUNTIME140/MSVCP140)
    ) else (
        echo NOTE: dumpbin not in PATH — install VS Build Tools or run from Developer Command Prompt.
        set RUNTIME_DEPS=static MSVC runtime /MT - no VC++ Redistributable required
    )
)

REM --- VERSION.txt ---
for /f "delims=" %%i in ('git rev-parse --short^=10 HEAD 2^>nul') do set GIT_SHA=%%i
for /f "delims=" %%i in ('git describe --tags --always --dirty 2^>nul') do set GIT_DESC=%%i
if not defined GIT_SHA set GIT_SHA=unknown
if not defined GIT_DESC set GIT_DESC=untagged

for /f "tokens=1-3 delims=/ " %%a in ('date /t') do set BUILD_DATE=%%c-%%a-%%b
echo AVIATION — PROTOTYPE 1 RC1> "%OUT_DIR%\VERSION.txt"
echo.>> "%OUT_DIR%\VERSION.txt"
echo Prototype version:     0.1.0-rc1>> "%OUT_DIR%\VERSION.txt"
echo Git commit:            %GIT_SHA%>> "%OUT_DIR%\VERSION.txt"
echo Git describe:          %GIT_DESC%>> "%OUT_DIR%\VERSION.txt"
echo Built:                 %BUILD_DATE%>> "%OUT_DIR%\VERSION.txt"
echo Platform:              Windows>> "%OUT_DIR%\VERSION.txt"
echo Architectures:         x86_64>> "%OUT_DIR%\VERSION.txt"
echo Plugin:                VST3 Instrument>> "%OUT_DIR%\VERSION.txt"
echo Plugin filename:       Aviation.vst3>> "%OUT_DIR%\VERSION.txt"
echo Manufacturer / code:   Avkz / Avk1>> "%OUT_DIR%\VERSION.txt"
echo JUCE:                  8.0.9>> "%OUT_DIR%\VERSION.txt"
echo Compiler:              Microsoft Visual Studio 2022>> "%OUT_DIR%\VERSION.txt"
echo Configuration:         Release>> "%OUT_DIR%\VERSION.txt"
echo Runtime deps:          %RUNTIME_DEPS%>> "%OUT_DIR%\VERSION.txt"
echo Validated hosts:       PENDING — FL11, FL20, FL21+, FL25 (64-bit x64; see PROTOTYPE1_FL_MATRIX.md)>> "%OUT_DIR%\VERSION.txt"
echo Minimum OS:            Windows 10 x64>> "%OUT_DIR%\VERSION.txt"
echo Status:                CONFIDENTIAL PROTOTYPE — NOT FOR REDISTRIBUTION>> "%OUT_DIR%\VERSION.txt"

echo.
echo Creating ZIP...
if exist "dist\%ZIP_NAME%" del "dist\%ZIP_NAME%"
powershell -NoProfile -Command "Compress-Archive -Path '%OUT_DIR%\*' -DestinationPath 'dist\%ZIP_NAME%' -Force"

echo.
echo == SHA-256 ==
certutil -hashfile "dist\%ZIP_NAME%" SHA256 > "%OUT_DIR%\SHA256SUMS.txt"
type "%OUT_DIR%\SHA256SUMS.txt"
copy /Y "%OUT_DIR%\SHA256SUMS.txt" "dist\SHA256SUMS.txt" >nul

echo.
echo ===================================================
echo  PACKAGE COMPLETE
echo  Folder: dist\%OUT_DIR%
echo  ZIP:    dist\%ZIP_NAME%
echo ===================================================
echo.
echo Next steps:
echo   1. Steinberg VST3 validator on VST3 bundle
echo   2. Clean Windows VM install test
echo   3. FL matrix — FL11, FL20, FL21+, FL25 (docs/PROTOTYPE1_FL_MATRIX.md)
echo   4. Archive ZIP + SHA256 + git SHA — do not modify ZIP after hash

endlocal
