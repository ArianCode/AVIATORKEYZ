@echo off
REM =============================================================================
REM  Aviation - Prototype 1 Windows packaging (universal bundle)
REM
REM  Prerequisites:
REM    scripts\build_windows_universal.bat        (x64 + x86 in one bundle)
REM      or scripts\build_windows_universal.bat x64only
REM
REM  Ships ONE Aviation.vst3 bundle. A 64-bit FL loads Contents\x86_64-win,
REM  a 32-bit FL 11 loads Contents\x86-win. One x64 binary already covers every
REM  Intel and AMD CPU on both Windows 10 and 11 - those are not separate builds.
REM
REM  Usage:
REM    scripts\package_prototype_windows.bat
REM    scripts\package_prototype_windows.bat --skip-dumpbin
REM =============================================================================

setlocal EnableDelayedExpansion

set ROOT=%~dp0..
cd /d "%ROOT%"

REM Keep RC_LABEL in step with AVIATORKEYZ_PROTOTYPE_VERSION in
REM cmake\AviatorKeyzBuildInfo.cmake. Bump both together or the artifact will
REM claim a version its binary does not carry.
set RC_LABEL=RC1
set PROTO_VERSION=0.1.0-rc1

set CONFIG=Release
set BUILD_DIR=build-win-x64
if not exist "%BUILD_DIR%" set BUILD_DIR=build

set OUT_DIR=dist\Aviation_Prototype1_%RC_LABEL%_Windows
set ZIP_NAME=Aviation_Prototype1_%RC_LABEL%_Windows.zip
set VST3_SRC=%BUILD_DIR%\Aviation_artefacts\%CONFIG%\VST3\Aviation.vst3
set DLL_X64=%VST3_SRC%\Contents\x86_64-win\Aviation.vst3
set DLL_X86=%VST3_SRC%\Contents\x86-win\Aviation.vst3
set SKIP_DUMPBIN=0

:parse_args
if "%~1"=="" goto done_args
if /I "%~1"=="--skip-dumpbin" set SKIP_DUMPBIN=1
shift
goto parse_args
:done_args

if not exist "%VST3_SRC%" (
    echo ERROR: Release VST3 not found at %VST3_SRC%
    echo        Run: scripts\build_windows_universal.bat
    exit /b 1
)

REM --- Architecture inventory -------------------------------------------------
set HAVE_X64=0
set HAVE_X86=0
if exist "%DLL_X64%" set HAVE_X64=1
if exist "%DLL_X86%" set HAVE_X86=1

if %HAVE_X64%==0 (
    echo ERROR: x86_64-win\Aviation.vst3 missing from the bundle.
    echo        The 64-bit half is mandatory - every 64-bit FL host needs it.
    exit /b 1
)

if %HAVE_X86%==1 (
    set ARCHS=x86_64 + x86
    set ARCH_NOTE=universal bundle - 64-bit FL loads x86_64-win, FL 11 32-bit loads x86-win
) else (
    set ARCHS=x86_64
    set ARCH_NOTE=64-bit only - FL 11 32-bit is NOT covered by this package
    echo.
    echo WARNING: no x86-win in the bundle. FL 11 32-bit will not see this plugin.
    echo          Re-run scripts\build_windows_universal.bat without x64only to include it.
    echo.
)

echo === Prototype 1 %RC_LABEL% packaging ^(%ARCHS%^) ===

for /f "delims=" %%i in ('git describe --tags --always --dirty 2^>nul') do set GIT_DESC_PRE=%%i
echo !GIT_DESC_PRE! | findstr /C:"dirty" >nul
if !errorlevel! equ 0 (
    echo ERROR: working tree is dirty. The recorded SHA would not rebuild this binary.
    echo        Commit, rebuild, and re-run before packaging an RC.
    exit /b 1
)
echo Commit identity: !GIT_DESC_PRE!

if exist "%OUT_DIR%" rmdir /s /q "%OUT_DIR%"
if not exist dist mkdir dist
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

REM --- Runtime dependencies ---------------------------------------------------
set RUNTIME_DEPS=static MSVC runtime /MT - no VC++ Redistributable required
if %SKIP_DUMPBIN%==0 (
    where dumpbin >nul 2>&1
    if !errorlevel! equ 0 (
        echo.
        echo == dumpbin /DEPENDENTS ^(x64^) ==
        dumpbin /DEPENDENTS "%DLL_X64%" > dist\dumpbin_dependents_x64.txt
        type dist\dumpbin_dependents_x64.txt
        if %HAVE_X86%==1 (
            echo.
            echo == dumpbin /DEPENDENTS ^(x86^) ==
            dumpbin /DEPENDENTS "%DLL_X86%" > dist\dumpbin_dependents_x86.txt
            type dist\dumpbin_dependents_x86.txt
        )
        set RUNTIME_DEPS=recorded in dist\dumpbin_dependents_*.txt ^(static CRT expected - no VCRUNTIME140/MSVCP140^)
    ) else (
        echo NOTE: dumpbin not in PATH - install VS Build Tools or run from a Developer Command Prompt.
    )
)

REM --- VERSION.txt ------------------------------------------------------------
for /f "delims=" %%i in ('git rev-parse --short^=10 HEAD 2^>nul') do set GIT_SHA=%%i
for /f "delims=" %%i in ('git describe --tags --always --dirty 2^>nul') do set GIT_DESC=%%i
if not defined GIT_SHA set GIT_SHA=unknown
if not defined GIT_DESC set GIT_DESC=untagged

for /f "tokens=1-3 delims=/ " %%a in ('date /t') do set BUILD_DATE=%%c-%%a-%%b
echo AVIATION - PROTOTYPE 1 %RC_LABEL%> "%OUT_DIR%\VERSION.txt"
echo.>> "%OUT_DIR%\VERSION.txt"
echo Prototype version:     %PROTO_VERSION%>> "%OUT_DIR%\VERSION.txt"
echo Git commit:            %GIT_SHA%>> "%OUT_DIR%\VERSION.txt"
echo Git describe:          %GIT_DESC%>> "%OUT_DIR%\VERSION.txt"
echo Built:                 %BUILD_DATE%>> "%OUT_DIR%\VERSION.txt"
echo Platform:              Windows 10 ^(1607+^) and Windows 11>> "%OUT_DIR%\VERSION.txt"
echo CPU:                   any x86-64 - Intel and AMD alike>> "%OUT_DIR%\VERSION.txt"
echo Architectures:         %ARCHS%>> "%OUT_DIR%\VERSION.txt"
echo Bundle layout:         %ARCH_NOTE%>> "%OUT_DIR%\VERSION.txt"
echo Plugin:                VST3 Instrument>> "%OUT_DIR%\VERSION.txt"
echo Plugin filename:       Aviation.vst3>> "%OUT_DIR%\VERSION.txt"
echo Manufacturer / code:   Avkz / Avk1>> "%OUT_DIR%\VERSION.txt"
echo JUCE:                  8.0.9>> "%OUT_DIR%\VERSION.txt"
echo Compiler:              Microsoft Visual Studio 2022>> "%OUT_DIR%\VERSION.txt"
echo Configuration:         Release>> "%OUT_DIR%\VERSION.txt"
echo Runtime deps:          %RUNTIME_DEPS%>> "%OUT_DIR%\VERSION.txt"
echo Validated hosts:       PENDING - FL11, FL20, FL21+, FL25 ^(see PROTOTYPE1_FL_MATRIX.md^)>> "%OUT_DIR%\VERSION.txt"
echo Status:                CONFIDENTIAL PROTOTYPE - NOT FOR REDISTRIBUTION>> "%OUT_DIR%\VERSION.txt"

echo.
echo Creating ZIP...
if exist "dist\%ZIP_NAME%" del "dist\%ZIP_NAME%"
powershell -NoProfile -Command "Compress-Archive -Path '%OUT_DIR%\*' -DestinationPath 'dist\%ZIP_NAME%' -Force"
if %errorlevel% neq 0 ( echo ERROR: ZIP creation failed. & exit /b 1 )

echo.
echo == SHA-256 ==
certutil -hashfile "dist\%ZIP_NAME%" SHA256 > "%OUT_DIR%\SHA256SUMS.txt"
type "%OUT_DIR%\SHA256SUMS.txt"
copy /Y "%OUT_DIR%\SHA256SUMS.txt" "dist\SHA256SUMS_Windows.txt" >nul

echo.
echo ===================================================
echo  PACKAGE COMPLETE  ^(%ARCHS%^)
echo  Folder: %OUT_DIR%
echo  ZIP:    dist\%ZIP_NAME%
echo ===================================================
echo.
echo Next steps:
echo   1. Steinberg VST3 validator on the bundle ^(run the x64 validator; also the
echo      x86 validator if x86-win is present^)
echo   2. Clean Windows VM install test - install to BOTH VST3 paths
echo   3. FL matrix - FL11 ^(note bitness^), FL20, FL21+, FL25
echo   4. Archive ZIP + SHA256 + git SHA - do not modify the ZIP after hashing

endlocal
