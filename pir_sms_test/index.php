<?php
/**
 * ============================================================
 * PIR + SMS TEST RIG - Dashboard
 * File: index.php
 * ============================================================
 *
 * Open it at:  http://localhost/ABMDMS/pir_sms_test/
 *
 * This page draws the shell once. After that, assets/dashboard.js
 * asks api/get_data.php for fresh numbers every 3 seconds and
 * fills them in, so the page updates without reloading.
 * ============================================================
 */

require_once __DIR__ . '/database.php';

// Check MySQL before drawing anything, so we can show a friendly
// warning strip instead of a page full of red PHP errors.
$dbCheck = testDBConnection();
?>
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>PIR + SMS Test Rig</title>
<link rel="stylesheet" href="assets/style.css">
</head>
<body>

<header class="navbar">
    <div class="brand">
        <span class="dot"></span>
        <div>
            <strong>PIR + SMS Test Rig</strong>
            <small>4 motion sensors &middot; 1 SIM800L &middot; isolated from the main system</small>
        </div>
    </div>
    <nav>
        <a href="wiring.html" target="_blank">Wiring Diagram</a>
        <a href="../index.php">Main ABMDMS</a>
    </nav>
</header>

<main class="wrap">

<?php if (!$dbCheck['ok']): ?>
    <div class="warning">
        <strong>Database problem.</strong>
        <?php echo htmlspecialchars($dbCheck['message'], ENT_QUOTES, 'UTF-8'); ?>
    </div>
<?php endif; ?>

    <!-- Offline strip: shown by dashboard.js when the API stops answering -->
    <div class="warning" id="offline" hidden>
        <strong>Not updating.</strong>
        Cannot reach <code>api/get_data.php</code>. Is Apache still running?
    </div>


    <!-- ============ BIG STATUS BANNER ============ -->
    <!-- Covers the whole rig: it reads MOTION if ANY room is busy. -->
    <section class="status" id="status-panel">
        <div class="status-label">Current Status</div>
        <div class="status-value" id="status-value">Loading&hellip;</div>
        <div class="status-meta">
            <?php echo count(ALLOWED_ZONES); ?> sensors on Arduino pins 2&ndash;5
            &middot; last updated <span id="last-updated">&mdash;</span>
        </div>
    </section>


    <!-- ============ ONE CARD PER ROOM ============ -->
    <!--
        Built from ALLOWED_ZONES in config.php, so adding a room
        there makes a card appear here on its own - nothing below
        is hardcoded to four. dashboard.js finds each card by its
        id ("zone-rooma") and switches its colour every 3 seconds.
    -->
    <section class="zones">
    <?php foreach (ALLOWED_ZONES as $zoneCode): ?>
        <article id="zone-<?php echo strtolower($zoneCode); ?>" class="zone-card zone-none">
            <div class="zone-head">
                <span class="zone-dot"></span>
                <span class="zone-label">
                    <?php echo htmlspecialchars(ZONE_LABELS[$zoneCode] ?? $zoneCode, ENT_QUOTES, 'UTF-8'); ?>
                </span>
            </div>
            <div class="zone-status">&mdash;</div>
            <div class="zone-meta">
                Last motion: <span class="zone-last">&mdash;</span><br>
                Last alert: <span class="zone-sms">&mdash;</span>
            </div>
        </article>
    <?php endforeach; ?>
    </section>


    <!-- ============ STAT TILES ============ -->
    <section class="tiles">
        <div class="tile">
            <div class="tile-label">Total Events</div>
            <div class="tile-value" id="stat-total">&mdash;</div>
            <div class="tile-note">motion start + stop, all time</div>
        </div>
        <div class="tile">
            <div class="tile-label">Today&rsquo;s Events</div>
            <div class="tile-value" id="stat-today">&mdash;</div>
            <div class="tile-note">since midnight</div>
        </div>
        <div class="tile">
            <div class="tile-label">SMS Sent Today</div>
            <div class="tile-value accent" id="stat-sms">&mdash;</div>
            <div class="tile-note">texts the network accepted</div>
        </div>
        <div class="tile">
            <div class="tile-label">Last Motion</div>
            <div class="tile-value small" id="stat-last">&mdash;</div>
            <div class="tile-note">last time movement started</div>
        </div>
    </section>


    <!-- ============ SMS ALERTS ============ -->
    <section class="panel">
        <div class="panel-head">
            <h2>SMS Alerts</h2>
            <div class="panel-sub">
                Alerts go to <strong><?php echo htmlspecialchars(SMS_RECIPIENT_DISPLAY, ENT_QUOTES, 'UTF-8'); ?></strong>
                &middot; last alert: <span id="sms-last">&mdash;</span>
            </div>
        </div>
        <div class="table-scroll">
            <table>
                <thead>
                    <tr>
                        <th>ID</th><th>Status</th><th>Zone</th><th>Detail</th><th>Date</th><th>Time</th>
                    </tr>
                </thead>
                <tbody id="sms-body">
                    <tr><td colspan="6" class="empty">Loading&hellip;</td></tr>
                </tbody>
            </table>
        </div>
        <p class="footnote">
            The Arduino sends the text itself through the SIM800L. This website
            never sends an SMS &mdash; it only records what the Arduino reports.
            <strong>SKIPPED</strong> means the 60-second cooldown blocked it on purpose.
            That cooldown is counted <strong>per room</strong>, so movement in a
            second room still texts you straight away.
        </p>
    </section>


    <!-- ============ MOTION HISTORY ============ -->
    <section class="panel">
        <div class="panel-head">
            <h2>Motion History</h2>
            <div class="panel-sub">Newest <?php echo (int) MOTION_RECENT_LIMIT; ?> events</div>
        </div>
        <div class="table-scroll">
            <table>
                <thead>
                    <tr>
                        <th>ID</th><th>Event</th><th>Zone</th><th>Source</th><th>Date</th><th>Time</th>
                    </tr>
                </thead>
                <tbody id="history-body">
                    <tr><td colspan="6" class="empty">Loading&hellip;</td></tr>
                </tbody>
            </table>
        </div>
    </section>

</main>

<footer class="foot">
    PIR + SMS Test Rig &middot; database <code>pir_sms_test</code> &middot;
    refreshes every 3 seconds
</footer>

<script src="assets/dashboard.js"></script>
</body>
</html>
