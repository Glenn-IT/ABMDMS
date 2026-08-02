# ============================================================
#  ABMDMS - Arduino Based Motion Detection Monitoring System
#  File: serial/serial_reader.ps1   (PowerShell serial bridge)
# ============================================================
#
#  WHAT THIS DOES
#  --------------
#  Reads the Arduino over USB, line by line, and passes every
#  event to the PHP API so it lands in MySQL.
#
#      Arduino -> USB -> THIS SCRIPT -> PHP API -> MySQL -> dashboard
#
#  It does the same job as serial_reader.php, but uses PowerShell's
#  built-in .NET serial support instead. PHP cannot open a COM port
#  on this PC, while PowerShell's always can - and PowerShell ships
#  with Windows, so nothing extra is needed.
#
#  WHAT IT LISTENS FOR
#  -------------------
#  (ROOMx is any of ROOMA, ROOMB, ROOMC. Room D / Pin 5 is removed -
#   see config.php. Add ROOMD back to both regexes to restore it.)
#
#  ROOMA_MOTION_DETECTED   -> api/record_motion.php
#  ROOMA_MOTION_STOPPED    -> api/record_motion.php
#  SMS_SENT:ROOMA          -> api/record_sms.php  (status SENT)
#  SMS_FAIL:ROOMA:TIMEOUT  -> api/record_sms.php  (status FAILED)
#  SMS_SKIP:ROOMA:COOLDOWN -> api/record_sms.php  (status SKIPPED)
#
#  Everything else the Arduino prints (warm-up countdown,
#  SIM_READY, System Ready...) is shown on screen but not saved.
#
#  This script does NOT send any text messages. The Arduino does.
#
#  HOW TO RUN IT
#  -------------
#  Just double-click:  start_reader.bat
#  Press Ctrl + C to stop it.
#
#  !! Close the Arduino IDE Serial Monitor before running. !!
#     Only one program can hold the COM port at a time.
# ============================================================


# ============================================================
#  >>>>>>>>>>  SETTINGS - EDIT THESE IF NEEDED  <<<<<<<<<<
# ============================================================

$ComPort   = "COM5"                                                # Your Arduino port
$BaudRate  = 9600                                                  # Must match Serial.begin(9600)
$ApiUrl    = "http://localhost/ABMDMS/api/record_motion.php"       # PHP API (motion)
$SmsApiUrl = "http://localhost/ABMDMS/api/record_sms.php"          # PHP API (SMS alerts)
$Source    = "ARDUINO_PIR"                                         # Saved with every event

$DuplicateWindow = 2     # Ignore the same event repeated within N seconds
$ReconnectDelay  = 3     # Seconds to wait before retrying a lost connection

# Matches "ROOMA_MOTION_DETECTED" -> zone=ROOMA, event=MOTION_DETECTED
# The room names are listed here on purpose: anything the Arduino
# prints that is NOT one of these is shown but never saved, so a
# garbled serial line can never invent a room.
$ZonePattern = '^(ROOMA|ROOMB|ROOMC)_(MOTION_DETECTED|MOTION_STOPPED)$'

# Matches "SMS_SENT:ROOMA" or "SMS_FAIL:ROOMB:TIMEOUT"
# -> result=SENT/FAIL/SKIP, zone=ROOMA, detail=TIMEOUT (detail optional)
$SmsPattern = '^SMS_(SENT|FAIL|SKIP):(ROOMA|ROOMB|ROOMC)(?::(.+))?$'

# ============================================================
#  You do not need to change anything below this line.
# ============================================================


# Small helper: print a line with a timestamp in front
function Say([string]$text) {
    Write-Host ("[" + (Get-Date -Format "HH:mm:ss") + "] " + $text)
}

function Line() { Write-Host ("-" * 58) }


# ------------------------------------------------------------
#  STARTUP BANNER
# ------------------------------------------------------------
Line
Write-Host "  ABMDMS - Arduino Serial Reader (PowerShell bridge)"
Line
Write-Host ("  COM port : " + $ComPort)
Write-Host ("  Baud rate: " + $BaudRate)
Write-Host ("  API URL  : " + $ApiUrl)
Line
Write-Host "  Press Ctrl + C to stop."
Write-Host "  Make sure the Arduino IDE Serial Monitor is CLOSED."
Line
Write-Host ""


# Remembers the last event so we do not save the same thing twice
$lastEvent = ""
$lastTime  = Get-Date "2000-01-01"


# ============================================================
#  MAIN LOOP
#  The outer loop reconnects automatically if the Arduino is
#  unplugged or the port is lost.
# ============================================================

while ($true) {

    $port = $null

    # --------------------------------------------------------
    #  STEP 1 - Open the COM port
    # --------------------------------------------------------
    try {
        $port = New-Object System.IO.Ports.SerialPort $ComPort, $BaudRate, ([System.IO.Ports.Parity]::None), 8, ([System.IO.Ports.StopBits]::One)
        $port.ReadTimeout = 500     # milliseconds
        $port.NewLine     = "`n"
        $port.Open()
    }
    catch {
        Say ("Could not open " + $ComPort + ": " + $_.Exception.Message)
        Say "  - Is the Arduino plugged in?"
        Say "  - Is the Arduino IDE Serial Monitor still open? Close it."
        Say "  - Is the COM port correct? (edit `$ComPort at the top)"
        Say ("Retrying in " + $ReconnectDelay + " seconds...")
        Write-Host ""
        Start-Sleep -Seconds $ReconnectDelay
        continue
    }

    Say ("Connected to " + $ComPort + ". Waiting for motion events...")
    Write-Host ""


    # --------------------------------------------------------
    #  STEP 2 - Read the Arduino, line by line, forever
    # --------------------------------------------------------
    while ($port.IsOpen) {

        # Read one line. If none arrives before the timeout, just
        # loop again (this also lets Ctrl + C work).
        try {
            $line = $port.ReadLine()
        }
        catch [System.TimeoutException] {
            continue
        }
        catch {
            # Any other error means the connection was lost
            break
        }

        $msg = $line.Trim()
        if ($msg -eq "") { continue }

        # Show everything the Arduino says (warm-up, System Ready, etc.)
        Write-Host ("    Arduino: " + $msg)

        # ----------------------------------------------------
        #  SMS ALERT REPORTS  (from the SIM800L on the Arduino)
        # ----------------------------------------------------
        #  The Arduino sends the text itself; it only TELLS us what
        #  happened. We save that report so the dashboard can show
        #  the alert really went out.
        #
        #  These deliberately skip the duplicate-guard further down:
        #  that guard exists to calm down PIR chatter. SMS reports
        #  are already rate-limited by the Arduino's per-zone
        #  cooldown, and each one is a genuinely separate alert.
        if ($msg -match $SmsPattern) {

            $smsResult = $Matches[1]
            $smsZone   = $Matches[2]
            $smsDetail = if ($Matches[3]) { $Matches[3] } else { "" }

            switch ($smsResult) {
                "SENT"  { $smsStatus = "SENT" }
                "FAIL"  { $smsStatus = "FAILED" }
                "SKIP"  { $smsStatus = "SKIPPED" }
                default { $smsStatus = "FAILED" }
            }

            try {
                $smsResponse = Invoke-RestMethod -Uri $SmsApiUrl -Method Post -TimeoutSec 5 -Body @{
                    zone   = $smsZone
                    status = $smsStatus
                    detail = $smsDetail
                }

                if ($smsResponse.success) {
                    Say ("[SMS " + $smsStatus + "] " + $smsZone + " -> saved as alert #" + $smsResponse.id)
                }
                else {
                    Say ("[SMS FAIL] " + $msg + " -> " + $smsResponse.message)
                }
            }
            catch {
                Say ("[SMS FAIL] " + $msg + " -> Cannot reach the API. Is Apache running? (" + $_.Exception.Message + ")")
            }

            continue
        }

        # Only zone-tagged motion tokens are real events we want to save
        if ($msg -notmatch $ZonePattern) {
            continue
        }
        $zone      = $Matches[1]
        $eventType = $Matches[2]

        # ----------------------------------------------------
        #  STEP 3 - Duplicate protection
        # ----------------------------------------------------
        # $msg already includes the zone (e.g. "ROOMA_MOTION_DETECTED"),
        # so this naturally guards per-zone - Room A repeating fast
        # never suppresses a real Room B event.
        $now = Get-Date
        if ($msg -eq $lastEvent -and ($now - $lastTime).TotalSeconds -lt $DuplicateWindow) {
            Say ("Skipped duplicate " + $msg)
            continue
        }
        $lastEvent = $msg
        $lastTime  = $now

        # ----------------------------------------------------
        #  STEP 4 - Send the event to the PHP API
        # ----------------------------------------------------
        try {
            $response = Invoke-RestMethod -Uri $ApiUrl -Method Post -TimeoutSec 5 -Body @{
                event_type = $eventType
                zone       = $zone
                source     = $Source
            }

            if ($response.success) {
                Say ("[OK]   " + $msg + " -> saved as record #" + $response.id)
            }
            else {
                Say ("[FAIL] " + $msg + " -> " + $response.message)
            }
        }
        catch {
            Say ("[FAIL] " + $msg + " -> Cannot reach the API. Is Apache running? (" + $_.Exception.Message + ")")
        }
    }

    # --------------------------------------------------------
    #  STEP 5 - The connection dropped (Arduino unplugged?)
    # --------------------------------------------------------
    if ($port -ne $null -and $port.IsOpen) { $port.Close() }

    Write-Host ""
    Say ("Lost the connection to " + $ComPort + ".")
    Say ("Reconnecting in " + $ReconnectDelay + " seconds...")
    Write-Host ""
    Start-Sleep -Seconds $ReconnectDelay
}
