<?php
/**
 * ============================================================
 * PIR + SMS TEST RIG  (isolated from the main ABMDMS system)
 * File: config.php
 * ============================================================
 *
 * This folder is a SMALL, SELF-CONTAINED version of ABMDMS:
 * one PIR sensor + one SIM800L, its own database, its own
 * dashboard. Nothing here touches the main system.
 *
 * Main system      -> database "motion_monitoring", 4 zones
 * This test rig    -> database "pir_sms_test",      1 zone
 *
 * That means you can break, re-import, or delete this rig
 * without losing any of the real ABMDMS data.
 * ============================================================
 */


// ------------------------------------------------------------
// DATABASE SETTINGS  (normal XAMPP defaults)
// ------------------------------------------------------------

define('DB_HOST', 'localhost');
define('DB_PORT', '3306');
define('DB_NAME', 'pir_sms_test');   // <-- its OWN database, not motion_monitoring
define('DB_USER', 'root');
define('DB_PASS', '');               // Default XAMPP password is empty
define('DB_CHARSET', 'utf8mb4');


// ------------------------------------------------------------
// TIME ZONE
// ------------------------------------------------------------

date_default_timezone_set('Asia/Manila');


// ------------------------------------------------------------
// ALLOWED VALUES  (security)
// ------------------------------------------------------------
// The API REFUSES to save anything not in these lists. This is
// what stops junk, SQL, or scripts from entering the database.

define('ALLOWED_EVENT_TYPES', ['MOTION_DETECTED', 'MOTION_STOPPED']);

// Only ONE sensor in this rig: the PIR on Arduino Pin 2.
define('ALLOWED_ZONES', ['ROOM1']);
define('ZONE_LABELS', ['ROOM1' => 'Room 1']);

define('ALLOWED_SMS_STATUSES', ['SENT', 'FAILED', 'SKIPPED']);


// ------------------------------------------------------------
// SMS SETTINGS
// ------------------------------------------------------------
// IMPORTANT: the Arduino sends the text itself through the
// SIM800L. PHP NEVER sends an SMS - it only writes down what
// the Arduino reports afterwards.
//
// To change the number that receives alerts, edit SMS_RECIPIENT
// inside arduino/pir_sms/pir_sms.ino and upload again. The line
// below is only what the dashboard displays.

define('SMS_RECIPIENT_DISPLAY', '+639169751409');

define('SMS_RECENT_LIMIT', 10);      // rows in the SMS Alerts table
define('MOTION_RECENT_LIMIT', 20);   // rows in the Motion History table


// ------------------------------------------------------------
// ERROR DISPLAY
// ------------------------------------------------------------

define('SHOW_ERRORS', true);

if (SHOW_ERRORS) {
    ini_set('display_errors', '1');
    error_reporting(E_ALL);
} else {
    ini_set('display_errors', '0');
    error_reporting(0);
}
