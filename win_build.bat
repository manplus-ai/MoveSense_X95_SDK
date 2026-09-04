@echo off
REM ============================================================
REM  Simou3Camera SDK - Windows multi-config build
REM  Builds with whichever of VS2017 / VS2019 is installed.
REM  Each: x64 + x32 (Win32), Debug + Release.
REM  Output layout:
REM     win_build\vs2017\x64\Release   win_build\vs2017\x64\Debug
REM     win_build\vs2017\x32\Release   win_build\vs2017\x32\Debug
REM     win_build\vs2019\x64\...        win_build\vs2019\x32\...
REM  If NEITHER VS2017 nor VS2019 is present -> error out.
REM  NOTE: comments/echo are ASCII on purpose (UTF-8 breaks cmd .bat).
REM ============================================================
setlocal enabledelayedexpansion
cd /d "%~dp0"

echo ============================================
echo   Simou3Camera SDK build (VS2017/VS2019 x64/x32 Debug/Release)
echo   Output: win_build\vsXXXX\{x64,x32}\{Release,Debug}
echo ============================================

REM ---- locate cmake: PATH first, then common install spots ----
set "CMAKE=cmake"
where cmake >nul 2>nul
if not errorlevel 1 goto have_cmake
set "CMAKE=C:\Program Files\CMake\bin\cmake.exe"
if exist "%CMAKE%" goto have_cmake
set "CMAKE=C:\Program Files (x86)\Microsoft Visual Studio\2017\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
if exist "%CMAKE%" goto have_cmake
echo [ERROR] cmake not found. Install CMake or add it to PATH.
exit /b 1

:have_cmake
echo [INFO] CMAKE = %CMAKE%

REM ---- fresh output tree ----
if exist win_build rd /s /q win_build

set BUILT=0
call :build_vs "Visual Studio 15 2017" vs2017
call :build_vs "Visual Studio 16 2019" vs2019

if "%BUILT%"=="0" (
    echo.
    echo [ERROR] Neither VS2017 nor VS2019 found - nothing was built.
    exit /b 1
)

echo.
echo ============================================
echo   BUILD OK. SDK libs under win_build\vsXXXX\arch\config\
echo     e.g. win_build\vs2017\x64\Release\Simou3CameraSDK.dll (+ .lib)
echo          win_build\vs2017\x64\Debug\Simou3CameraSDKD.dll  (+ .lib)
echo ============================================
exit /b 0

REM ==================== subroutines ====================

:build_vs
REM %1 = generator, %2 = tag(vs2017/vs2019)
call :build_arch %1 %2 x64 x64
call :build_arch %1 %2 x32 Win32
goto :eof

:build_arch
REM %1 = generator, %2 = tag, %3 = dir(x64/x32), %4 = cmake arch(x64/Win32)
set "GEN=%~1"
set "TAG=%~2"
set "ADIR=%~3"
set "AARCH=%~4"
set "BDIR=win_build\%TAG%\%ADIR%"

REM configure (quiet): failure here means this VS/arch is not installed -> skip
"%CMAKE%" -S . -B "%BDIR%" -G "%GEN%" -A %AARCH% >nul 2>nul
if errorlevel 1 (
    echo   [skip] %TAG% %ADIR%  ^(generator not available^)
    if exist "%BDIR%" rd /s /q "%BDIR%"
    goto :eof
)

echo   [build] %TAG% %ADIR%  Release + Debug
"%CMAKE%" --build "%BDIR%" --config Release --target Simou3CameraSDK
if errorlevel 1 ( echo   [ERROR] %TAG% %ADIR% Release FAILED & goto :eof )
"%CMAKE%" --build "%BDIR%" --config Debug --target Simou3CameraSDK
if errorlevel 1 ( echo   [ERROR] %TAG% %ADIR% Debug FAILED & goto :eof )

set BUILT=1
goto :eof
