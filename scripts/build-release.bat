@echo off
REM =============================================================
REM Yalaz Engine - Release Build Script (Windows)
REM =============================================================
REM Usage: scripts\build-release.bat

setlocal EnableDelayedExpansion

cd /d "%~dp0\.."

echo ========================================
echo Yalaz Engine - Release Build (Windows)
echo ========================================
echo.

REM Check for CMake
where cmake >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo Error: CMake not found in PATH
    echo Please install CMake and add it to your PATH
    exit /b 1
)

echo Step 1: Configure...
cmake --preset package
if %ERRORLEVEL% NEQ 0 (
    echo Configuration failed!
    exit /b 1
)

echo.
echo Step 2: Build...
cmake --build --preset package-build --parallel
if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    exit /b 1
)

echo.
echo Step 3: Package...
cd build\package
cpack -G ZIP
if %ERRORLEVEL% NEQ 0 (
    echo Packaging failed!
    exit /b 1
)

echo.
echo ========================================
echo Build complete!
echo Package location: build\package\
echo ========================================

dir *.zip *.exe 2>nul

endlocal
