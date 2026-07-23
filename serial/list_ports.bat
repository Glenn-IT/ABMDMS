@echo off
REM ============================================================
REM  ABMDMS - Show which COM ports exist on this computer
REM ============================================================
REM  Double-click this file to find your Arduino's COM port.
REM
REM  TIP: Run it once with the Arduino UNPLUGGED, then plug the
REM  Arduino in and run it again. The port that appeared the
REM  second time is your Arduino.
REM ============================================================

title ABMDMS - COM Port Finder
color 0B

echo ============================================================
echo   COM PORTS CURRENTLY AVAILABLE ON THIS COMPUTER
echo ============================================================
echo.

reg query "HKLM\HARDWARE\DEVICEMAP\SERIALCOMM" 2>nul

echo.
echo ============================================================
echo   Look for a line ending in COM3, COM4, COM5 and so on.
echo   That number goes into serial_reader.php on this line:
echo.
echo       $COM_PORT = "COM3";
echo.
echo   If NOTHING is listed above, the Arduino is not detected.
echo   Check the USB cable and the Arduino drivers.
echo ============================================================
echo.

pause
