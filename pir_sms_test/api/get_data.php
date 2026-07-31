<?php
/**
 * ============================================================
 * PIR + SMS TEST RIG
 * File: api/get_data.php
 * ============================================================
 *
 * WHAT THIS DOES
 * --------------
 * Gives the dashboard EVERYTHING it needs in one request, so
 * the page only has to ask once every few seconds.
 *
 * HOW TO CALL IT
 * --------------
 * GET http://localhost/ABMDMS/pir_sms_test/api/get_data.php
 *
 * WHAT IT SENDS BACK
 * ------------------
 * {
 *   "success": true,
 *   "status":  "MOTION" | "CLEAR" | "NO_DATA",
 *   "stats":   { total_events, today_events, sms_today, last_motion },
 *   "sms":     { recipient, last, recent: [...] },
 *   "motion":  { recent: [...] },
 *   "server_time": "14:05:11"
 * }
 * ============================================================
 */

require_once __DIR__ . '/../database.php';

header('Content-Type: application/json; charset=utf-8');


try {
    $db    = getDB();
    $today = date('Y-m-d');


    // --------------------------------------------------------
    // 1. CURRENT STATUS
    //    The newest motion row tells us where we stand right now.
    //    MOTION_DETECTED = someone is there. MOTION_STOPPED = clear.
    // --------------------------------------------------------

    $stmt = $db->query(
        'SELECT event_type, detected_at
           FROM motion_events
       ORDER BY detected_at DESC, id DESC
          LIMIT 1'
    );
    $latest = $stmt->fetch();

    if ($latest === false) {
        $status = 'NO_DATA';
    } elseif ($latest['event_type'] === 'MOTION_DETECTED') {
        $status = 'MOTION';
    } else {
        $status = 'CLEAR';
    }


    // --------------------------------------------------------
    // 2. COUNTERS
    // --------------------------------------------------------

    $totalEvents = (int) $db->query('SELECT COUNT(*) FROM motion_events')->fetchColumn();

    $stmt = $db->prepare('SELECT COUNT(*) FROM motion_events WHERE DATE(detected_at) = ?');
    $stmt->execute([$today]);
    $todayEvents = (int) $stmt->fetchColumn();

    // Only texts that actually went out count as "sent today".
    $stmt = $db->prepare("SELECT COUNT(*) FROM sms_events WHERE status = 'SENT' AND DATE(sent_at) = ?");
    $stmt->execute([$today]);
    $smsToday = (int) $stmt->fetchColumn();

    // The last time movement STARTED (ignore the stop events).
    $stmt = $db->query(
        "SELECT detected_at
           FROM motion_events
          WHERE event_type = 'MOTION_DETECTED'
       ORDER BY detected_at DESC, id DESC
          LIMIT 1"
    );
    $lastMotion = $stmt->fetchColumn();


    // --------------------------------------------------------
    // 3. THE LAST SMS  (any status, so failures show up too)
    // --------------------------------------------------------

    $stmt = $db->query(
        'SELECT status, detail, sent_at
           FROM sms_events
       ORDER BY sent_at DESC, id DESC
          LIMIT 1'
    );
    $lastSms = $stmt->fetch();


    // --------------------------------------------------------
    // 4. RECENT ROWS FOR THE TWO TABLES
    // --------------------------------------------------------
    // LIMIT values come from config.php constants (whole numbers
    // we control), never from the user, so they are safe to inline.

    $stmt = $db->query(
        'SELECT id, zone, status, detail, sent_at
           FROM sms_events
       ORDER BY sent_at DESC, id DESC
          LIMIT ' . (int) SMS_RECENT_LIMIT
    );
    $recentSms = $stmt->fetchAll();

    $stmt = $db->query(
        'SELECT id, event_type, zone, source, detected_at
           FROM motion_events
       ORDER BY detected_at DESC, id DESC
          LIMIT ' . (int) MOTION_RECENT_LIMIT
    );
    $recentMotion = $stmt->fetchAll();


    // --------------------------------------------------------
    // 5. SEND IT ALL BACK
    // --------------------------------------------------------

    echo json_encode([
        'success' => true,
        'status'  => $status,
        'stats'   => [
            'total_events' => $totalEvents,
            'today_events' => $todayEvents,
            'sms_today'    => $smsToday,
            'last_motion'  => $lastMotion !== false ? $lastMotion : null,
        ],
        'sms' => [
            'recipient' => SMS_RECIPIENT_DISPLAY,
            'last'      => $lastSms !== false ? $lastSms : null,
            'recent'    => $recentSms,
        ],
        'motion' => [
            'recent' => $recentMotion,
        ],
        'server_time' => date('H:i:s'),
    ]);

} catch (PDOException $e) {

    error_log('PIR_SMS_TEST get_data error: ' . $e->getMessage());

    http_response_code(500);
    echo json_encode([
        'success' => false,
        'message' => 'Cannot read the database. Is MySQL running, and did you import '
                   . 'database/pir_sms_test.sql in phpMyAdmin?',
    ]);
}
