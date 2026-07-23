@echo off
REM ============================================================
REM  ABMDMS - Start the Arduino Serial Reader (PowerShell bridge)
REM ============================================================
REM  Just double-click this file.
REM
REM  Before running:
REM    1. XAMPP Apache and MySQL must be running.
REM    2. The Arduino must be plugged in.
REM    3. The Arduino IDE Serial Monitor must be CLOSED.
REM ============================================================

title ABMDMS Serial Reader
color 0B

cd /d "%~dp0"

echo Starting ABMDMS serial reader...
echo.

powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0serial_reader.ps1"

echo.
echo ============================================================
echo  The serial reader has stopped.
echo ============================================================
pause
