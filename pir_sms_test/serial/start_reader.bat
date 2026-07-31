@echo off
REM ============================================================
REM  PIR + SMS TEST RIG - Start the Arduino Serial Reader
REM ============================================================
REM  Just double-click this file.
REM
REM  Before running:
REM    1. XAMPP Apache and MySQL must be running.
REM    2. database/pir_sms_test.sql must be imported already.
REM    3. The Arduino must be plugged in.
REM    4. The Arduino IDE Serial Monitor must be CLOSED.
REM ============================================================

title PIR + SMS Test Rig - Serial Reader
color 0B

cd /d "%~dp0"

echo Starting the PIR + SMS serial reader...
echo.

powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0serial_reader.ps1"

echo.
echo ============================================================
echo  The serial reader has stopped.
echo ============================================================
pause
