<?php
/**
 * ============================================================
 * ABMDMS - Arduino Based Motion Detection Monitoring System
 * File: tools/simulate_motion.php
 * ============================================================
 *
 * WHAT THIS DOES
 * --------------
 * This page pretends to be the Arduino.
 *
 * Clicking a button sends a real motion event to the real API,
 * which saves it in the real database. It is the fastest way
 * to prove the website half of the system works before you
 * even touch the hardware.
 *
 * Open it at:
 *     http://localhost/ABMDMS/tools/simulate_motion.php
 *
 * Tip: open the dashboard in another browser tab at the same
 * time and watch it update by itself.
 * ============================================================
 */

require_once __DIR__ . '/../config.php';
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ABMDMS - Test Tool</title>
    <link rel="stylesheet" href="../assets/css/style.css">
</head>
<body>

<header class="navbar">
    <div class="navbar-inner">
        <div class="brand">
            <span class="brand-dot"></span>
            <div>
                <h1>ABMDMS</h1>
                <p>Test Tool &mdash; simulate the Arduino</p>
            </div>
        </div>
        <nav class="nav-links">
            <a href="../index.php">Dashboard</a>
            <a href="simulate_motion.php" class="active">Test Tool</a>
        </nav>
    </div>
</header>


<main class="container">

    <div class="alert alert-error" style="background:rgba(245,158,11,.12);border-color:rgba(245,158,11,.4);color:#fde68a;">
        <strong>For testing only.</strong>
        Events created here are saved with the source
        <code>SIMULATOR</code> so you can tell them apart from real
        Arduino readings.
    </div>


    <section class="panel">

        <div class="panel-head">
            <h2>Simulate a Motion Event</h2>
            <span class="muted">Sends to api/record_motion.php</span>
        </div>

        <ol class="steps">
            <li>Open the <a href="../index.php" target="_blank">dashboard</a> in a second tab.</li>
            <li>Click a button below.</li>
            <li>Watch the dashboard update by itself within about 3 seconds.</li>
        </ol>

        <!-- The room list comes from ALLOWED_ZONES in config.php, so it
             always matches what the dashboard and the API accept. Without
             this the API falls back to its default room and every
             simulated event lands in the same card. -->
        <div class="tool-actions">
            <label for="zone-select" class="muted">Room</label>
            <select id="zone-select" class="btn">
<?php foreach (ALLOWED_ZONES as $zoneCode): ?>
                <option value="<?= htmlspecialchars($zoneCode) ?>"><?= htmlspecialchars(ZONE_LABELS[$zoneCode] ?? $zoneCode) ?></option>
<?php endforeach; ?>
            </select>
        </div>

        <div class="tool-actions">
            <button class="btn btn-primary"   data-event="MOTION_DETECTED">Simulate MOTION_DETECTED</button>
            <button class="btn btn-secondary" data-event="MOTION_STOPPED">Simulate MOTION_STOPPED</button>
            <button class="btn"               id="check-api">Check API Connection</button>
        </div>

        <pre class="tool-output" id="output">Ready. Click a button above to send a test event.</pre>

    </section>


    <footer class="footer">
        ABMDMS &middot; Test Tool
    </footer>

</main>


<script>
(function () {
    'use strict';

    var API_RECORD = '../api/record_motion.php';
    var API_LOGS   = '../api/get_motion_logs.php';
    var output     = document.getElementById('output');

    /* Adds one timestamped line to the black output box */
    function log(text) {
        var stamp = new Date().toLocaleTimeString();
        output.textContent = '[' + stamp + '] ' + text + '\n' + output.textContent;
    }

    /* Sends one fake motion event to the real API */
    function sendEvent(eventType) {

        var zone = document.getElementById('zone-select').value;

        log('Sending ' + eventType + ' for ' + zone + ' ...');

        var body = new URLSearchParams();
        body.append('event_type', eventType);
        body.append('zone', zone);
        body.append('source', 'SIMULATOR');

        fetch(API_RECORD, { method: 'POST', body: body })
            .then(function (r) { return r.json(); })
            .then(function (data) {
                if (data.success) {
                    log('SUCCESS - ' + data.message + ' (record #' + data.id + ' at ' + data.detected_at + ')');
                } else {
                    log('FAILED - ' + data.message);
                }
            })
            .catch(function (err) {
                log('ERROR - could not reach the API. Is Apache running? (' + err.message + ')');
            });
    }

    /* Wire up both simulate buttons */
    var buttons = document.querySelectorAll('[data-event]');
    for (var i = 0; i < buttons.length; i++) {
        buttons[i].addEventListener('click', function () {
            sendEvent(this.getAttribute('data-event'));
        });
    }

    /* Quick health check of the read API */
    document.getElementById('check-api').addEventListener('click', function () {
        log('Checking API...');

        fetch(API_LOGS, { cache: 'no-store' })
            .then(function (r) { return r.json(); })
            .then(function (data) {
                if (data.success) {
                    log('API OK - status: ' + data.status +
                        ', total events: ' + data.total_events +
                        ', records in database: ' + data.total_rows);
                } else {
                    log('API returned an error - ' + data.message);
                }
            })
            .catch(function (err) {
                log('ERROR - ' + err.message);
            });
    });

})();
</script>

</body>
</html>
