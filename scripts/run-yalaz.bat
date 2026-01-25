@echo off
REM =============================================================
REM Yalaz Engine Launcher (Windows)
REM =============================================================
REM Double-click this file to start the engine

setlocal

set "SCRIPT_DIR=%~dp0"
cd /d "%SCRIPT_DIR%"

REM Find and launch the executable
if exist "%SCRIPT_DIR%bin\Yalaz_Engine.exe" (
    start "" "%SCRIPT_DIR%bin\Yalaz_Engine.exe" %*
) else (
    for %%f in ("%SCRIPT_DIR%bin\Yalaz_Engine-*.exe") do (
        start "" "%%f" %*
        goto :end
    )
    echo Error: Could not find Yalaz_Engine executable
    pause
)

:end
endlocal
