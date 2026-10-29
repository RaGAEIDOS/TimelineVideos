@echo off
REM Run this on WINDOWS, inside the project folder (double-click it).
REM Requires: Python 3 installed and on PATH. Nothing else —
REM the app plays videos through whatever app Windows already
REM has set as default, so there's no engine to bundle.

REM Always run from the folder this .bat file is in, no matter how
REM it was launched.
cd /d "%~dp0"

setlocal

echo === Timeline Videos - Windows EXE builder ===
echo.

if not exist "assets\logo.ico" (
    echo ERROR: assets\logo.ico not found next to this script.
    echo Make sure the "assets" folder ^(with logo.ico and icon_mark.png^)
    echo sits in the SAME folder as build_windows.bat and main.py.
    pause
    exit /b 1
)

python -m venv venv
call venv\Scripts\activate.bat

pip install --upgrade pip
pip install -r requirements.txt
pip install pyinstaller

pyinstaller video_player.spec

if errorlevel 1 (
    echo.
    echo ================================================
    echo BUILD FAILED - see the error message above.
    echo ================================================
    pause
    exit /b 1
)

echo.
echo Done. TimelineVideos.exe is in the "dist" folder.
pause
