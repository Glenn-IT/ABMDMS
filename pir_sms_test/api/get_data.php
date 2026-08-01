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
 *   "status":  "MOTION" | "CLEAR" | "NO_DATA",   <- the whole rig
 *   "zones":   { "ROOMA": { label, status, last_motion, last_sms }, ... },
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
    // 1. WHERE EACH ROOM STANDS RIGHT NOW
    //    For every room, look up its OWN newest row. That row's
    //    event type is that room's current state - MOTION_DETECTED
    //    means somebody is in there, MOTION_STOPPED means it is clear.
    //
    //    It has to be per room. Asking only for the newest row in
    //    the whole table would let a "stopped" in Room B hide a
    //    person still moving in Room A.
    // --------------------------------------------------------

    // Prepared once, run once per room - same query, different room.
    $zoneStmt = $db->prepare(
        'SELECT event_type, detected_at
           FROM motion_events
          WHERE zone = ?
       ORDER BY detected_at DESC, id DESC
          LIMIT 1'
    );

    $zoneSmsStmt = $db->prepare(
        'SELECT sent_at
           FROM sms_events
          WHERE zone = ? AND status = ?
       ORDER BY sent_at DESC, id DESC
          LIMIT 1'
    );

    $zones      = [];
    $anyMotion  = false;
    $anyData    = false;

    foreach (ALLOWED_ZONES as $zoneCode) {

        $zoneStmt->execute([$zoneCode]);
        $zoneRow = $zoneStmt->fetch();

        $zoneSmsStmt->execute([$zoneCode, 'SENT']);
        $zoneSms = $zoneSmsStmt->fetchColumn();

        if ($zoneRow === false) {
            $zoneStatus = 'NO_DATA';
        } elseif ($zoneRow['event_type'] === 'MOTION_DETECTED') {
            $zoneStatus = 'MOTION';
            $anyMotion  = true;
        } else {
            $zoneStatus = 'CLEAR';
        }

        if ($zoneRow !== false) {
            $anyData = true;
        }

        $zones[$zoneCode] = [
            'label'       => ZONE_LABELS[$zoneCode] ?? $zoneCode,
            'status'      => $zoneStatus,
            'last_motion' => $zoneRow !== false ? $zoneRow['detected_at'] : null,
            'last_sms'    => $zoneSms !== false ? $zoneSms : null,
        ];
    }

    // The banner at the top of the page: MOTION if ANY room is busy.
    if (!$anyData) {
        $status = 'NO_DATA';
    } elseif ($anyMotion) {
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
        'zones'   => $zones,
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
