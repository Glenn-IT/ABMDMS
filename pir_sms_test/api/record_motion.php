<?php
/**
 * ============================================================
 * PIR + SMS TEST RIG
 * File: api/record_motion.php
 * ============================================================
 *
 * WHAT THIS DOES
 * --------------
 * The Arduino prints a line like "ROOM1_MOTION_DETECTED" over
 * USB. The PowerShell serial reader turns that into a request
 * to THIS file, and this file saves it into MySQL.
 *
 * HOW TO CALL IT
 * --------------
 * POST http://localhost/ABMDMS/pir_sms_test/api/record_motion.php
 *
 *     event_type = MOTION_DETECTED   (or MOTION_STOPPED)
 *     zone       = ROOM1
 *     source     = ARDUINO_PIR       (optional)
 *
 * ANSWER
 * ------
 * {"success": true,  "message": "Motion event recorded", "id": 12, ...}
 * {"success": false, "message": "..."}
 * ============================================================
 */

require_once __DIR__ . '/../database.php';

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

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    respond(false, 'Unable to record motion: this endpoint only accepts POST requests.', [], 405);
}


// ------------------------------------------------------------
// STEP 2 - Read the incoming data
// ------------------------------------------------------------

$eventType = isset($_POST['event_type']) ? trim((string) $_POST['event_type']) : '';
$zone      = isset($_POST['zone'])       ? trim((string) $_POST['zone'])       : 'ROOM1';
$source    = isset($_POST['source'])     ? trim((string) $_POST['source'])     : 'ARDUINO_PIR';


// ------------------------------------------------------------
// STEP 3 - Validate the data  (SECURITY)
// ------------------------------------------------------------

// 3a. The event type must be one we allow.
if ($eventType === '') {
    respond(false, 'Unable to record motion: event_type is missing.', [], 400);
}
$eventType = strtoupper($eventType);

if (!in_array($eventType, ALLOWED_EVENT_TYPES, true)) {
    respond(false, 'Unable to record motion: invalid event_type.', [], 400);
}

// 3b. The zone must be one we allow (this rig only knows ROOM1).
$zone = strtoupper($zone);

if (!in_array($zone, ALLOWED_ZONES, true)) {
    respond(false, 'Unable to record motion: invalid zone.', [], 400);
}

// 3c. Clean the free-text source and keep it short.
$source = strtoupper(substr(preg_replace('/[^A-Za-z0-9_\-]/', '', $source), 0, 50));
if ($source === '') {
    $source = 'ARDUINO_PIR';
}


// ------------------------------------------------------------
// STEP 4 - Save it into the database
// ------------------------------------------------------------

try {
    $db = getDB();

    $detectedAt = date('Y-m-d H:i:s');

    // A PREPARED STATEMENT. PDO fills the ? marks in safely, so
    // nobody can inject SQL commands through the data.
    $sql = 'INSERT INTO motion_events (event_type, zone, source, detected_at) VALUES (?, ?, ?, ?)';

    $stmt = $db->prepare($sql);
    $stmt->execute([$eventType, $zone, $source, $detectedAt]);

    respond(true, 'Motion event recorded', [
        'id'          => (int) $db->lastInsertId(),
        'event_type'  => $eventType,
        'zone'        => $zone,
        'source'      => $source,
        'detected_at' => $detectedAt,
    ]);

} catch (PDOException $e) {

    // Log the real reason for us, show nothing technical outside.
    error_log('PIR_SMS_TEST record_motion error: ' . $e->getMessage());

    respond(false, 'Unable to record motion event', [], 500);
}
