<?php
/**
 * ============================================================
 * ABMDMS - Arduino Based Motion Detection Monitoring System
 * File: config.php
 * ============================================================
 *
 * This is the ONLY file you need to edit if your setup is
 * different (for example if your MySQL has a password).
 *
 * Every other file in the project reads its settings from here.
 * ============================================================
 */


// ------------------------------------------------------------
// DATABASE SETTINGS
// These are the normal XAMPP defaults.
// ------------------------------------------------------------

define('DB_HOST', 'localhost');            // Where MySQL is running
define('DB_PORT', '3306');                 // Default MySQL port
define('DB_NAME', 'motion_monitoring');    // Database created by database.sql
define('DB_USER', 'root');                 // Default XAMPP username
define('DB_PASS', '');                     // Default XAMPP password is empty
define('DB_CHARSET', 'utf8mb4');


// ------------------------------------------------------------
// TIME ZONE
// ------------------------------------------------------------
// Set this to your own time zone so the dashboard shows the
// correct clock time. Philippines = 'Asia/Manila'.
// Full list: https://www.php.net/manual/en/timezones.php

date_default_timezone_set('Asia/Manila');


// ------------------------------------------------------------
// ALLOWED EVENT TYPES  (security)
// ------------------------------------------------------------
// The API will REFUSE to save anything that is not in this list.
// This stops junk or malicious data from entering the database.

define('ALLOWED_EVENT_TYPES', ['MOTION_DETECTED', 'MOTION_STOPPED']);

// The API will REFUSE to save a zone that is not in this list too.
// ROOMC is the original/existing PIR sensor (Pin 2); ROOMA, ROOMB, and
// ROOMD are the sensors added on the breadboard (Pin 3, Pin 4, Pin 5).
define('ALLOWED_ZONES', ['ROOMA', 'ROOMB', 'ROOMC', 'ROOMD']);

// Friendly names shown on the dashboard for each zone code above.
define('ZONE_LABELS', [
    'ROOMA' => 'Room A',
    'ROOMB' => 'Room B',
    'ROOMC' => 'Room C',
    'ROOMD' => 'Room D',
]);


// ------------------------------------------------------------
// SMS ALERT SETTINGS  (SIM800L EVB)
// ------------------------------------------------------------
// IMPORTANT: the Arduino sends the text messages itself, straight
// through the SIM800L module. PHP never sends anything - it only
// RECORDS what the Arduino reports, so the dashboard can show it.
//
// To change the phone number that receives alerts you must edit
// SMS_RECIPIENT inside arduino/motion_sensor/motion_sensor.ino
// and upload the sketch again. The setting below is only the
// number shown on the dashboard.

define('SMS_RECIPIENT_DISPLAY', '+639171234567');

// The API will REFUSE to save an SMS status that is not in this list.
//   SENT    = the network accepted the message
//   FAILED  = the module could not send it
//   SKIPPED = blocked on purpose by the Arduino's per-zone cooldown
define('ALLOWED_SMS_STATUSES', ['SENT', 'FAILED', 'SKIPPED']);

// How many recent alerts the dashboard's "SMS Alerts" panel shows.
define('SMS_RECENT_LIMIT', 10);


// ------------------------------------------------------------
// DASHBOARD SETTINGS
// ------------------------------------------------------------

define('RECORDS_PER_PAGE', 20);   // Rows shown in the history table
define('MAX_RECORDS_PER_PAGE', 100);


// ------------------------------------------------------------
// ERROR DISPLAY
// ------------------------------------------------------------
// While you are building the project, leave this as true so you
// can see PHP errors on screen. Set it to false before a demo.

define('SHOW_ERRORS', true);

if (SHOW_ERRORS) {
    ini_set('display_errors', '1');
    error_reporting(E_ALL);
} else {
    ini_set('display_errors', '0');
    error_reporting(0);
}
