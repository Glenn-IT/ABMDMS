<?php
/**
 * ============================================================
 * ABMDMS - Arduino Based Motion Detection Monitoring System
 * File: api/record_motion.php
 * ============================================================
 *
 * WHAT THIS DOES
 * --------------
 * This is the "mailbox" of the system. The serial reader sends
 * a motion event here, and this file saves it into MySQL.
 *
 * HOW TO CALL IT
 * --------------
 * POST http://localhost/ABMDMS/api/record_motion.php
 *
 * Data to send:
 *     event_type = MOTION_DETECTED
 *     zone       = ROOMA          (ROOMA, ROOMB or ROOMC — defaults to ROOMC)
 *     source     = ARDUINO_PIR
 *
 * WHAT IT SENDS BACK
 * ------------------
 * Success:
 *     {"success": true,  "message": "Motion recorded successfully", ...}
 * Failure:
 *     {"success": false, "message": "Unable to record motion"}
 * ============================================================
 */

require_once __DIR__ . '/../database.php';

// Tell the browser / script that our answer is JSON, not HTML
header('Content-Type: application/json; charset=utf-8');


/**
 * Prints a JSON answer and stops the script immediately.
 */
function respond(bool $success, string $message, array $extra = [], int $httpCode = 200): void
{
    http_response_code($httpCode);
    echo json_encode(array_merge([
        'success' => $success,
        'message' => $message,
    ], $extra));
    exit;
}


// ------------------------------------------------------------
// STEP 1 - Only allow POST requests
// ------------------------------------------------------------
// Saving data should never happen from a plain browser visit,
// so we reject anything that is not POST.

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    respond(false, 'Unable to record motion: this endpoint only accepts POST requests.', [], 405);
}


// ------------------------------------------------------------
// STEP 2 - Read the incoming data
// ------------------------------------------------------------

$eventType = isset($_POST['event_type']) ? trim((string) $_POST['event_type']) : '';
$zone      = isset($_POST['zone'])       ? trim((string) $_POST['zone'])       : 'ROOMC';
$source    = isset($_POST['source'])     ? trim((string) $_POST['source'])     : 'ARDUINO_PIR';


// ------------------------------------------------------------
// STEP 3 - Validate the data  (SECURITY)
// ------------------------------------------------------------

// 3a. The event type must not be empty
if ($eventType === '') {
    respond(false, 'Unable to record motion: event_type is missing.', [], 400);
}

// 3b. The event type must be one we actually allow.
//     Anything else (junk, SQL, scripts) is thrown away here.
$eventType = strtoupper($eventType);

if (!in_array($eventType, ALLOWED_EVENT_TYPES, true)) {
    respond(false, 'Unable to record motion: invalid event_type.', [], 400);
}

// 3c. The zone must also be one we actually allow.
if ($zone === '') {
    $zone = 'ROOMC';
}
$zone = strtoupper($zone);

if (!in_array($zone, ALLOWED_ZONES, true)) {
    respond(false, 'Unable to record motion: invalid zone.', [], 400);
}

// 3d. Clean up the source label and keep it short
if ($source === '') {
    $source = 'ARDUINO_PIR';
}
$source = strtoupper(substr(preg_replace('/[^A-Za-z0-9_\- ]/', '', $source), 0, 50));


// ------------------------------------------------------------
// STEP 4 - Save it into the database
// ------------------------------------------------------------

try {
    $db = getDB();

    // The time the motion happened, using the time zone in config.php
    $detectedAt = date('Y-m-d H:i:s');

    // A PREPARED STATEMENT. The ? marks are filled in safely by PDO,
    // so no one can inject SQL commands through the data.
    $sql = 'INSERT INTO motion_logs (event_type, zone, source, detected_at) VALUES (?, ?, ?, ?)';

    $stmt = $db->prepare($sql);
    $stmt->execute([$eventType, $zone, $source, $detectedAt]);

    $newId = (int) $db->lastInsertId();

    respond(true, 'Motion recorded successfully', [
        'id'          => $newId,
        'event_type'  => $eventType,
        'zone'        => $zone,
        'source'      => $source,
        'detected_at' => $detectedAt,
    ]);

} catch (PDOException $e) {

    // Write the real reason into the Apache error log for us,
    // but never show database details to the outside world.
    error_log('ABMDMS record_motion error: ' . $e->getMessage());

    respond(false, 'Unable to record motion', [], 500);
}
