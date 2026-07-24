<?php
/**
 * ============================================================
 * ABMDMS - Arduino Based Motion Detection Monitoring System
 * File: serial/serial_reader.php
 * ============================================================
 *
 * WHAT THIS DOES
 * --------------
 * This is the BRIDGE between the Arduino and the website.
 *
 *   Arduino  ->  USB cable  ->  THIS SCRIPT  ->  PHP API  ->  MySQL
 *
 * It listens to the COM port, and every time the Arduino prints
 * a zone-tagged token like ROOMA_MOTION_DETECTED it splits that
 * into zone=ROOMA + event_type=MOTION_DETECTED and sends both to
 * the PHP API, which saves it in the database.
 *
 * HOW TO RUN IT
 * -------------
 * Just double-click:   start_reader.bat
 *
 * Or from a command prompt:
 *     C:\xampp\php\php.exe -f "C:\xampp\htdocs\ABMDMS\serial\serial_reader.php"
 *
 * Press Ctrl + C to stop it.
 *
 * !! IMPORTANT !!
 * Close the Arduino IDE Serial Monitor before starting this.
 * Only ONE program can use the COM port at a time.
 * ============================================================
 */


/* ============================================================
   >>>>>>>>>>  SETTINGS - EDIT THESE THREE LINES  <<<<<<<<<<
   ============================================================ */

$COM_PORT  = 'COM5';   // <-- Your Arduino port. Run list_ports.bat to find it.
$BAUD_RATE = 9600;     // <-- Must match Serial.begin(9600) in the Arduino code.
$API_URL   = 'http://localhost/ABMDMS/api/record_motion.php';

$SOURCE            = 'ARDUINO_PIR';  // Label saved with every event
$DUPLICATE_WINDOW  = 2;              // Ignore the same event repeated within N seconds
$RECONNECT_DELAY   = 3;              // Seconds to wait before retrying a lost connection
$TIMEZONE          = 'Asia/Manila';  // Must match the one in config.php

/* ============================================================
   You do not need to change anything below this line.
   ============================================================ */


// This script is only meant to be run from the command line,
// never opened in a browser.
if (PHP_SAPI !== 'cli') {
    header('Content-Type: text/plain');
    exit("This file must be run from the command line.\r\n"
       . "Double-click serial/start_reader.bat instead.\r\n");
}

// Never time out - this script is supposed to run forever.
set_time_limit(0);

// Use the same clock as the website, so the times printed in this
// window match the times shown on the dashboard.
date_default_timezone_set($TIMEZONE);


// ------------------------------------------------------------
// SMALL HELPERS
// ------------------------------------------------------------

/** Prints a line with a timestamp in front of it. */
function say(string $text): void
{
    echo '[' . date('H:i:s') . '] ' . $text . PHP_EOL;
}

/** Prints a divider line. */
function line(): void
{
    echo str_repeat('-', 58) . PHP_EOL;
}

/**
 * Windows needs a special name for ports numbered 10 and above.
 * COM3  ->  COM3:
 * COM12 ->  \\.\COM12
 * This is the number one reason a bridge silently fails to open.
 */
function buildPortPath(string $port): string
{
    $number = (int) filter_var($port, FILTER_SANITIZE_NUMBER_INT);

    return ($number >= 10) ? '\\\\.\\' . $port : $port . ':';
}


/**
 * Sends one motion event to the PHP API using cURL.
 *
 * @return array{ok: bool, message: string}
 */
function sendToApi(string $apiUrl, string $eventType, string $zone, string $source): array
{
    $ch = curl_init($apiUrl);

    curl_setopt_array($ch, [
        CURLOPT_POST           => true,
        CURLOPT_POSTFIELDS     => http_build_query([
            'event_type' => $eventType,
            'zone'       => $zone,
            'source'     => $source,
        ]),
        CURLOPT_RETURNTRANSFER => true,
        CURLOPT_TIMEOUT        => 5,
        CURLOPT_CONNECTTIMEOUT => 3,
    ]);

    $response = curl_exec($ch);
    $curlErr  = curl_error($ch);
    $httpCode = (int) curl_getinfo($ch, CURLINFO_HTTP_CODE);

    curl_close($ch);

    // Could not reach Apache at all
    if ($response === false) {
        return ['ok' => false, 'message' => 'Cannot reach the API (' . $curlErr . '). Is Apache running?'];
    }

    $json = json_decode($response, true);

    if (!is_array($json)) {
        return ['ok' => false, 'message' => 'API sent back an unexpected reply (HTTP ' . $httpCode . ').'];
    }

    if (empty($json['success'])) {
        return ['ok' => false, 'message' => $json['message'] ?? 'API refused the event.'];
    }

    return ['ok' => true, 'message' => 'saved as record #' . ($json['id'] ?? '?')];
}


// ------------------------------------------------------------
// STARTUP BANNER
// ------------------------------------------------------------

line();
echo "  ABMDMS - Arduino Serial Reader (PHP bridge)" . PHP_EOL;
line();
echo "  COM port : {$COM_PORT}"  . PHP_EOL;
echo "  Baud rate: {$BAUD_RATE}" . PHP_EOL;
echo "  API URL  : {$API_URL}"   . PHP_EOL;
line();
echo "  Press Ctrl + C to stop." . PHP_EOL;
echo "  Make sure the Arduino IDE Serial Monitor is CLOSED." . PHP_EOL;
line();
echo PHP_EOL;


// Remembers the last event so we do not save the same thing twice
$lastEventType = null;
$lastEventTime = 0;


// ============================================================
// MAIN LOOP
// The outer loop reconnects automatically if the Arduino is
// unplugged or the port is lost.
// ============================================================

while (true) {

    // --------------------------------------------------------
    // STEP 1 - Configure the COM port using the built-in
    //          Windows "mode" command.
    // --------------------------------------------------------

    $modeCommand = sprintf(
        'mode %s BAUD=%d PARITY=N DATA=8 STOP=1 xon=off odsr=off octs=off rts=on',
        $COM_PORT . ':',
        $BAUD_RATE
    );

    exec($modeCommand . ' 2>&1', $modeOutput, $modeStatus);

    if ($modeStatus !== 0) {
        say("Could not configure {$COM_PORT}.");
        say('  - Is the Arduino plugged in?');
        say('  - Is the port name correct? Run list_ports.bat to check.');
        say('  - Is the Arduino IDE Serial Monitor still open? Close it.');
        say("Retrying in {$RECONNECT_DELAY} seconds...");
        echo PHP_EOL;
        sleep($RECONNECT_DELAY);
        continue;   // go back to the top and try again
    }


    // --------------------------------------------------------
    // STEP 2 - Open the port so we can read from it
    // --------------------------------------------------------

    $portPath = buildPortPath($COM_PORT);
    $handle   = @fopen($portPath, 'r+b');

    if ($handle === false) {
        say("Could not open {$COM_PORT}. Another program may be using it.");
        say("Retrying in {$RECONNECT_DELAY} seconds...");
        echo PHP_EOL;
        sleep($RECONNECT_DELAY);
        continue;
    }

    say("Connected to {$COM_PORT}. Waiting for motion events...");
    echo PHP_EOL;


    // --------------------------------------------------------
    // STEP 3 - Read the Arduino, line by line, forever
    // --------------------------------------------------------

    while (!feof($handle)) {

        $rawLine = fgets($handle);

        // No data right now - wait a moment and look again
        if ($rawLine === false) {
            usleep(100000);   // 0.1 second
            continue;
        }

        // Clean up spaces and the invisible line-ending characters
        $message = trim($rawLine);

        if ($message === '') {
            continue;
        }

        // Show EVERYTHING the Arduino says, so you can see the
        // warm-up countdown and the "System Ready" message too.
        echo '    Arduino: ' . $message . PHP_EOL;

        // Is this line a zone-tagged motion token, e.g. ROOMA_MOTION_DETECTED?
        if (!preg_match('/^(ROOMA|ROOMB|ROOMC|ROOMD)_(MOTION_DETECTED|MOTION_STOPPED)$/', $message, $m)) {
            continue;   // just a status message, nothing to save
        }
        $zone      = $m[1];
        $eventType = $m[2];


        // ----------------------------------------------------
        // STEP 4 - Duplicate protection
        // ----------------------------------------------------
        // If the exact same event arrives again within a couple
        // of seconds, ignore it so the database stays clean.
        // $message already includes the zone, so Room A repeating
        // fast never suppresses a real Room B event.

        $now = time();

        if ($message === $lastEventType && ($now - $lastEventTime) < $DUPLICATE_WINDOW) {
            say("Skipped duplicate {$message}");
            continue;
        }

        $lastEventType = $message;
        $lastEventTime = $now;


        // ----------------------------------------------------
        // STEP 5 - Send the event to the PHP API
        // ----------------------------------------------------

        $result = sendToApi($API_URL, $eventType, $zone, $SOURCE);

        if ($result['ok']) {
            say("[OK]   {$message} -> " . $result['message']);
        } else {
            say("[FAIL] {$message} -> " . $result['message']);
        }
    }


    // --------------------------------------------------------
    // STEP 6 - The connection dropped (Arduino unplugged?)
    // --------------------------------------------------------

    fclose($handle);

    echo PHP_EOL;
    say("Lost the connection to {$COM_PORT}.");
    say("Reconnecting in {$RECONNECT_DELAY} seconds...");
    echo PHP_EOL;

    sleep($RECONNECT_DELAY);
}
