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
 * assets/js/dashboard.js keeps the numbers up to date every
 * few seconds WITHOUT reloading the page.
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


    <!-- ========================================================
         BIG STATUS INDICATOR
         ======================================================== -->
    <section id="status-panel" class="status-panel status-none">
        <div class="status-light"></div>
        <div class="status-text">
            <span class="status-label">Current Status</span>
            <span id="status-value" class="status-value">NO MOTION</span>
        </div>
        <div class="status-meta">
            <span>Last updated</span>
            <strong id="last-updated">--:--:--</strong>
        </div>
    </section>


    <!-- ========================================================
         PER-ZONE STATUS (Room A / Room B / Room C / Room D)
         ======================================================== -->
    <section class="zones">
<?php foreach (ALLOWED_ZONES as $zoneCode): ?>
        <article id="zone-<?= strtolower($zoneCode) ?>" class="zone-card zone-none">
            <span class="zone-dot"></span>
            <div class="zone-text">
                <span class="zone-label"><?= htmlspecialchars(ZONE_LABELS[$zoneCode] ?? $zoneCode) ?></span>
                <span class="zone-value">NO MOTION</span>
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
            <span id="card-status" class="card-value">NO MOTION</span>
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
    </footer>

</main>

<script src="assets/js/dashboard.js"></script>
</body>
</html>
