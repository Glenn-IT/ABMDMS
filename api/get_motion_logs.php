<?php
/**
 * ============================================================
 * ABMDMS - Arduino Based Motion Detection Monitoring System
 * File: api/get_motion_logs.php
 * ============================================================
 *
 * WHAT THIS DOES
 * --------------
 * The dashboard asks this file for fresh information every few
 * seconds. It returns EVERYTHING the dashboard needs in one
 * single answer, so the page only makes one request per update.
 *
 * HOW TO CALL IT
 * --------------
 * GET http://localhost/ABMDMS/api/get_motion_logs.php
 * GET http://localhost/ABMDMS/api/get_motion_logs.php?page=2
 *
 * WHAT IT SENDS BACK
 * ------------------
 * {
 *   "success": true,
 *   "status": "NO_MOTION",
 *   "zones": {
 *     "ROOMA": { "label": "Room A", "status": "NO_MOTION", "last_motion": "..." },
 *     "ROOMB": { "label": "Room B", "status": "MOTION",    "last_motion": "..." },
 *     "ROOMC": { "label": "Room C", "status": "NO_MOTION", "last_motion": "..." }
 *   },
 *   "total_events": 125,
 *   "today_events": 25,
 *   "last_motion": "July 24, 2026 12:30 PM",
 *   "logs": [ { "id":1, "event_type":"MOTION_DETECTED", "zone":"ROOMA", ... } ],
 *   "page": 1,
 *   "total_pages": 7
 * }
 * ============================================================
 */

require_once __DIR__ . '/../database.php';

header('Content-Type: application/json; charset=utf-8');

// Stop the browser from showing an old cached copy
header('Cache-Control: no-store, no-cache, must-revalidate');


try {
    $db = getDB();

    // --------------------------------------------------------
    // STEP 1 - Work out which page of history was requested
    // --------------------------------------------------------

    // (int) forces the value to a whole number, so ?page=abc
    // becomes 0 and cannot break the SQL.
    $page = isset($_GET['page']) ? (int) $_GET['page'] : 1;
    if ($page < 1) {
        $page = 1;
    }

    $limit = isset($_GET['limit']) ? (int) $_GET['limit'] : RECORDS_PER_PAGE;
    if ($limit < 1) {
        $limit = RECORDS_PER_PAGE;
    }
    if ($limit > MAX_RECORDS_PER_PAGE) {
        $limit = MAX_RECORDS_PER_PAGE;
    }

    $offset = ($page - 1) * $limit;


    // --------------------------------------------------------
    // STEP 2 - Count how many records exist in total
    // --------------------------------------------------------

    $totalRows = (int) $db->query('SELECT COUNT(*) FROM motion_logs')->fetchColumn();

    $totalPages = ($totalRows > 0) ? (int) ceil($totalRows / $limit) : 1;

    // If the user asked for page 99 but there are only 3 pages,
    // send them back to the last real page.
    if ($page > $totalPages) {
        $page   = $totalPages;
        $offset = ($page - 1) * $limit;
    }


    // --------------------------------------------------------
    // STEP 3 - The statistics for the summary cards
    // --------------------------------------------------------

    // 3a. Total number of times motion was DETECTED
    $stmt = $db->prepare('SELECT COUNT(*) FROM motion_logs WHERE event_type = ?');
    $stmt->execute(['MOTION_DETECTED']);
    $totalEvents = (int) $stmt->fetchColumn();

    // 3b. How many of those happened today
    $stmt = $db->prepare(
        'SELECT COUNT(*) FROM motion_logs
         WHERE event_type = ? AND DATE(detected_at) = ?'
    );
    $stmt->execute(['MOTION_DETECTED', date('Y-m-d')]);
    $todayEvents = (int) $stmt->fetchColumn();

    // 3c. When was motion last detected?
    $stmt = $db->prepare(
        'SELECT detected_at FROM motion_logs
         WHERE event_type = ?
         ORDER BY id DESC LIMIT 1'
    );
    $stmt->execute(['MOTION_DETECTED']);
    $lastMotionRaw = $stmt->fetchColumn();

    $lastMotion = $lastMotionRaw
        ? date('F j, Y g:i A', strtotime($lastMotionRaw))
        : 'No motion yet';


    // --------------------------------------------------------
    // STEP 4 - Current status (is someone there RIGHT NOW?)
    // --------------------------------------------------------
    // We look at the newest record of any kind, overall AND per zone.
    //   newest = MOTION_DETECTED -> someone is there
    //   newest = MOTION_STOPPED  -> the area is clear

    $newestEvent = $db->query(
        'SELECT event_type FROM motion_logs ORDER BY id DESC LIMIT 1'
    )->fetchColumn();

    $status = ($newestEvent === 'MOTION_DETECTED') ? 'MOTION' : 'NO_MOTION';

    // 4a. Same thing, but one status per zone.
    $zoneStmt = $db->prepare(
        'SELECT event_type, detected_at FROM motion_logs
         WHERE zone = ?
         ORDER BY id DESC LIMIT 1'
    );

    $zones = [];
    foreach (ALLOWED_ZONES as $zoneCode) {
        $zoneStmt->execute([$zoneCode]);
        $zoneRow = $zoneStmt->fetch();

        $zones[$zoneCode] = [
            'label'       => ZONE_LABELS[$zoneCode] ?? $zoneCode,
            'status'      => ($zoneRow && $zoneRow['event_type'] === 'MOTION_DETECTED') ? 'MOTION' : 'NO_MOTION',
            'last_motion' => $zoneRow ? date('F j, Y g:i A', strtotime($zoneRow['detected_at'])) : 'No motion yet',
        ];
    }


    // --------------------------------------------------------
    // STEP 5 - The history table rows (newest first)
    // --------------------------------------------------------

    $stmt = $db->prepare(
        'SELECT id, event_type, zone, source, detected_at
         FROM motion_logs
         ORDER BY id DESC
         LIMIT :limit OFFSET :offset'
    );

    // LIMIT and OFFSET must be bound as real integers
    $stmt->bindValue(':limit',  $limit,  PDO::PARAM_INT);
    $stmt->bindValue(':offset', $offset, PDO::PARAM_INT);
    $stmt->execute();

    $logs = [];

    foreach ($stmt->fetchAll() as $row) {
        $time = strtotime($row['detected_at']);

        $logs[] = [
            'id'         => (int) $row['id'],
            'event_type' => $row['event_type'],
            'zone'       => $row['zone'],
            'zone_label' => ZONE_LABELS[$row['zone']] ?? $row['zone'],
            'source'     => $row['source'],
            'date'       => date('M j, Y', $time),   // Jul 24, 2026
            'time'       => date('g:i:s A', $time),  // 12:30:05 PM
        ];
    }


    // --------------------------------------------------------
    // STEP 6 - Send everything back as JSON
    // --------------------------------------------------------

    echo json_encode([
        'success'      => true,
        'status'       => $status,
        'zones'        => $zones,
        'total_events' => $totalEvents,
        'today_events' => $todayEvents,
        'last_motion'  => $lastMotion,
        'logs'         => $logs,
        'page'         => $page,
        'total_pages'  => $totalPages,
        'total_rows'   => $totalRows,
        'server_time'  => date('g:i:s A'),
    ]);

} catch (PDOException $e) {

    error_log('ABMDMS get_motion_logs error: ' . $e->getMessage());

    http_response_code(500);
    echo json_encode([
        'success' => false,
        'message' => 'Unable to load motion logs. Check that MySQL is running.',
    ]);
}
