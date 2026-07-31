@echo off
REM ============================================================
REM  PIR + SMS TEST RIG - Which COM port is the Arduino on?
REM ============================================================
REM  Double-click this, find the line that says "Arduino Uno",
REM  and note its COMx number. If it is not COM5, edit the
REM  $ComPort setting at the top of serial_reader.ps1.
REM ============================================================

title PIR + SMS Test Rig - COM Ports
color 0B

echo Looking for serial ports...
echo.

powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "Get-CimInstance Win32_PnPEntity | Where-Object { $_.Name -match '\(COM\d+\)' } | Select-Object -ExpandProperty Name"

echo.
echo ============================================================
pause
