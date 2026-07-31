<?php
/**
 * ============================================================
 * PIR + SMS TEST RIG
 * File: api/record_sms.php
 * ============================================================
 *
 * WHAT THIS DOES
 * --------------
 * The twin of record_motion.php, but for SMS alerts.
 *
 * The Arduino sends the text itself through the SIM800L. It
 * then prints a short report over USB, e.g. "SMS_SENT:ROOM1".
 * The serial reader passes that to THIS file, which writes it
 * down so the dashboard can prove the alert really went out.
 *
 * This file NEVER sends an SMS.
 *
 * HOW TO CALL IT
 * --------------
 * POST http://localhost/ABMDMS/pir_sms_test/api/record_sms.php
 *
 *     zone      = ROOM1
 *     status    = SENT       (SENT, FAILED, or SKIPPED)
 *     detail    = TIMEOUT    (optional short reason, may be empty)
 *     recipient = +639...    (optional, defaults to config)
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
    respond(false, 'Unable to record SMS alert: this endpoint only accepts POST requests.', [], 405);
}


// ------------------------------------------------------------
// STEP 2 - Read the incoming data
// ------------------------------------------------------------

$zone      = isset($_POST['zone'])      ? trim((string) $_POST['zone'])      : 'ROOM1';
$status    = isset($_POST['status'])    ? trim((string) $_POST['status'])    : '';
$detail    = isset($_POST['detail'])    ? trim((string) $_POST['detail'])    : '';
$recipient = isset($_POST['recipient']) ? trim((string) $_POST['recipient']) : SMS_RECIPIENT_DISPLAY;


// ------------------------------------------------------------
// STEP 3 - Validate the data  (SECURITY)
// ------------------------------------------------------------

// 3a. The zone must be one we allow.
$zone = strtoupper($zone);

if (!in_array($zone, ALLOWED_ZONES, true)) {
    respond(false, 'Unable to record SMS alert: invalid zone.', [], 400);
}

// 3b. The status must be one we allow. Junk, SQL, and scripts
//     are all thrown away right here.
if ($status === '') {
    respond(false, 'Unable to record SMS alert: status is missing.', [], 400);
}
$status = strtoupper($status);

if (!in_array($status, ALLOWED_SMS_STATUSES, true)) {
    respond(false, 'Unable to record SMS alert: invalid status.', [], 400);
}

// 3c. Clean up the free-text bits and keep them short.
$detail    = strtoupper(substr(preg_replace('/[^A-Za-z0-9_\- ]/', '', $detail), 0, 100));
$recipient = substr(preg_replace('/[^0-9+]/', '', $recipient), 0, 20);


// ------------------------------------------------------------
// STEP 4 - Save it into the database
// ------------------------------------------------------------

try {
    $db = getDB();

    $sentAt = date('Y-m-d H:i:s');

    $sql = 'INSERT INTO sms_events (zone, recipient, status, detail, sent_at) VALUES (?, ?, ?, ?, ?)';

    $stmt = $db->prepare($sql);
    $stmt->execute([$zone, $recipient, $status, $detail, $sentAt]);

    respond(true, 'SMS alert recorded', [
        'id'        => (int) $db->lastInsertId(),
        'zone'      => $zone,
        'recipient' => $recipient,
        'status'    => $status,
        'detail'    => $detail,
        'sent_at'   => $sentAt,
    ]);

} catch (PDOException $e) {

    error_log('PIR_SMS_TEST record_sms error: ' . $e->getMessage());

    respond(false, 'Unable to record SMS alert', [], 500);
}
