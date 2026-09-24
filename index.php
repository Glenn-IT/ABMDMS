<?php
/**
 * ============================================================
 * ABMDMS - Arduino Based Motion Detection Monitoring System
 * File: index.php   (this IS the dashboard)
 * ============================================================
 *
 * Open it in your browser at:
 *     http://localhost/ABMDMS/
 *
 * This page draws the layout once. After that, the file
 * assets/js/dashboard.js asks api/get_motion_logs.php for fresh
 * numbers every 3 seconds and fills them in, so the dashboard
 * updates WITHOUT reloading the page.
 * ============================================================
 */

require_once __DIR__ . '/database.php';

// Check the database before drawing anything, so we can show a
// friendly warning instead of a page full of red error text.
$dbCheck = testDBConnection();
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ABMDMS - Motion Monitoring Dashboard</title>
    <link rel="stylesheet" href="assets/css/style.css">
</head>
<body>

<!-- ============================================================
     TOP NAVIGATION BAR
     ============================================================ -->
<header class="navbar">
    <div class="navbar-inner">
        <div class="brand">
            <span class="brand-dot"></span>
            <div>
                <h1>ABMDMS</h1>
                <p>Arduino Based Motion Detection Monitoring System</p>
            </div>
        </div>

        <nav class="nav-links">
            <a href="index.php" class="active">Dashboard</a>
            <a href="wiring.html" target="_blank">Wiring Diagram</a>
            <a href="prototype.php" target="_blank">System Prototype</a>
            <a href="tools/simulate_motion.php">Test Tool</a>
            <span id="connection-badge" class="badge badge-live">Live</span>
        </nav>
    </div>
</header>


<main class="container">

<?php if (!$dbCheck['ok']): ?>
    <!-- Shown only when MySQL is unreachable -->
    <div class="alert alert-error">
        <strong>Database problem.</strong>
        <?= htmlspecialchars($dbCheck['message']) ?>
    </div>
<?php endif; ?>

    <!-- Shown by dashboard.js when the API stops answering.
         Separate from the badge: the badge says "we lost the
         server", this says what to check about it. -->
    <div class="alert alert-error" id="offline" hidden>
        <strong>Not updating.</strong>
        Cannot reach <code>api/get_motion_logs.php</code>. Is Apache still running?
    </div>


    <!-- ========================================================
         BIG STATUS INDICATOR
         Covers the whole system: it reads MOTION if ANY room is busy.
         ======================================================== -->
    <section id="status-panel" class="status-panel status-none">
        <div class="status-light"></div>
        <div class="status-text">
            <span class="status-label">Current Status</span>
            <span id="status-value" class="status-value">Loading&hellip;</span>
        </div>
        <div class="status-meta">
            <span><?= count(ALLOWED_ZONES) ?> sensors on Arduino pins 2&ndash;4</span>
            <strong>Last updated <span id="last-updated">--:--:--</span></strong>
        </div>
    </section>


    <!-- ========================================================
         ONE CARD PER ROOM

         Built from ALLOWED_ZONES in config.php, so adding or
         removing a room there changes this section on its own -
         nothing below is hardcoded to a number of rooms.
         dashboard.js finds each card by its id ("zone-rooma").
         ======================================================== -->
    <section class="zones">
<?php foreach (ALLOWED_ZONES as $zoneCode): ?>
        <article id="zone-<?= strtolower($zoneCode) ?>" class="zone-card zone-none">
            <div class="zone-head">
                <span class="zone-dot"></span>
                <span class="zone-label"><?= htmlspecialchars(ZONE_LABELS[$zoneCode] ?? $zoneCode) ?></span>
            </div>
            <div class="zone-value">&mdash;</div>
            <div class="zone-meta">
                Last motion: <span class="zone-last">&mdash;</span><br>
                Last alert: <span class="zone-sms">&mdash;</span>
            </div>
        </article>
<?php endforeach; ?>
    </section>


    <!-- ========================================================
         SUMMARY CARDS
         ======================================================== -->
    <section class="cards">

        <article class="card">
            <span class="card-label">Current Status</span>
            <span id="card-status" class="card-value">&mdash;</span>
            <span class="card-hint">Live sensor state</span>
        </article>

        <article class="card">
            <span class="card-label">Total Events</span>
            <span id="card-total" class="card-value">0</span>
            <span class="card-hint">All motion detections</span>
        </article>

        <article class="card">
            <span class="card-label">Today's Events</span>
            <span id="card-today" class="card-value">0</span>
            <span class="card-hint">Detected today</span>
        </article>

        <article class="card">
            <span class="card-label">Last Motion</span>
            <span id="card-last" class="card-value card-value-small">No motion yet</span>
            <span class="card-hint">Most recent detection</span>
        </article>

    </section>


    <!-- ========================================================
         SMS ALERTS  (sent by the Arduino through the SIM800L)
         ======================================================== -->
    <section class="panel">

        <div class="panel-head">
            <h2>SMS Alerts</h2>
            <span class="muted">Sent to <?= htmlspecialchars(SMS_RECIPIENT_DISPLAY) ?></span>
        </div>

        <div class="cards">

            <article class="card">
                <span class="card-label">Alerts Today</span>
                <span id="sms-today" class="card-value">0</span>
                <span class="card-hint">Text messages sent today</span>
            </article>

            <article class="card">
                <span class="card-label">Last Alert</span>
                <span id="sms-last" class="card-value card-value-small">No alerts yet</span>
                <span class="card-hint">Most recent message sent</span>
            </article>

        </div>

        <div class="table-wrap">
            <table class="table">
                <thead>
                    <tr>
                        <th>ID</th>
                        <th>Status</th>
                        <th>Zone</th>
                        <th>Detail</th>
                        <th>Date</th>
                        <th>Time</th>
                    </tr>
                </thead>
                <tbody id="sms-body">
                    <tr>
                        <td colspan="6" class="empty">Loading...</td>
                    </tr>
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


    <!-- ========================================================
         MOTION HISTORY TABLE
         ======================================================== -->
    <section class="panel">

        <div class="panel-head">
            <h2>Motion History</h2>
            <span id="record-count" class="muted">0 records</span>
        </div>

        <div class="table-wrap">
            <table class="table">
                <thead>
                    <tr>
                        <th>ID</th>
                        <th>Event Type</th>
                        <th>Zone</th>
                        <th>Source</th>
                        <th>Date</th>
                        <th>Time</th>
                    </tr>
                </thead>
                <tbody id="history-body">
                    <tr>
                        <td colspan="6" class="empty">Loading...</td>
                    </tr>
                </tbody>
            </table>
        </div>

        <!-- Pagination buttons -->
        <div class="pagination">
            <button id="prev-page" class="btn" disabled>&laquo; Previous</button>
            <span id="page-info" class="muted">Page 1 of 1</span>
            <button id="next-page" class="btn" disabled>Next &raquo;</button>
        </div>

    </section>


    <footer class="footer">
        ABMDMS &middot; Arduino Uno + HC-SR501 PIR + SIM800L SMS &middot; XAMPP / PHP / MySQL
        &middot; database <code><?= htmlspecialchars(DB_NAME) ?></code>
        &middot; refreshes every 3 seconds
    </footer>

</main>

<script src="assets/js/dashboard.js"></script>
</body>
</html>
