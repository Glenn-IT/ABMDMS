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
// ROOMC is the original/existing PIR sensor (Pin 2); ROOMA and ROOMB
// are the 2 new sensors added on the breadboard (Pin 3 and Pin 4).
define('ALLOWED_ZONES', ['ROOMA', 'ROOMB', 'ROOMC']);

// Friendly names shown on the dashboard for each zone code above.
define('ZONE_LABELS', [
    'ROOMA' => 'Room A',
    'ROOMB' => 'Room B',
    'ROOMC' => 'Room C',
]);


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
