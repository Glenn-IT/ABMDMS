/* ============================================================
   PIR + SMS TEST RIG - Dashboard refresher
   File: assets/dashboard.js
   ============================================================

   Every 3 seconds this asks api/get_data.php for the latest
   numbers and writes them into the page. The page itself is
   never reloaded, so the browser does not flicker.
   ============================================================ */

(function () {
    'use strict';

    var API        = 'api/get_data.php';
    var REFRESH_MS = 3000;

    // Grab the elements once, instead of searching on every refresh
    var el = {
        panel:   document.getElementById('status-panel'),
        status:  document.getElementById('status-value'),
        updated: document.getElementById('last-updated'),
        offline: document.getElementById('offline'),
        total:   document.getElementById('stat-total'),
        today:   document.getElementById('stat-today'),
        sms:     document.getElementById('stat-sms'),
        last:    document.getElementById('stat-last'),
        smsLast: document.getElementById('sms-last'),
        smsBody: document.getElementById('sms-body'),
        history: document.getElementById('history-body')
    };

    // The room cards. index.php already drew one per room from
    // ALLOWED_ZONES, so we just collect whatever is on the page -
    // this code never needs to know how many rooms there are.
    var zoneCards = {};
    Array.prototype.forEach.call(
        document.querySelectorAll('.zone-card'),
        function (card) {
            // id="zone-rooma"  ->  key "ROOMA", matching the API
            zoneCards[card.id.replace('zone-', '').toUpperCase()] = card;
        }
    );


    // --------------------------------------------------------
    // Small helpers
    // --------------------------------------------------------

    // Turn any text into something safe to drop into HTML.
    // Even though this data comes from our own database, escaping
    // it means a weird value can never break the page.
    function safe(text) {
        return String(text === null || text === undefined ? '' : text)
            .replace(/&/g, '&amp;')
            .replace(/</g, '&lt;')
            .replace(/>/g, '&gt;')
            .replace(/"/g, '&quot;');
    }

    // "2026-08-01 14:05:11"  ->  { date: "2026-08-01", time: "14:05:11" }
    function splitStamp(stamp) {
        if (!stamp) { return { date: '—', time: '—' }; }
        var parts = String(stamp).split(' ');
        return { date: parts[0] || '—', time: parts[1] || '—' };
    }

    function emptyRow(tbody, columns, message) {
        tbody.innerHTML = '<tr><td colspan="' + columns + '" class="empty">' + message + '</td></tr>';
    }


    // --------------------------------------------------------
    // Drawing
    // --------------------------------------------------------

    function drawStatus(status) {
        el.panel.classList.remove('is-motion', 'is-clear', 'is-nodata');

        if (status === 'MOTION') {
            el.panel.classList.add('is-motion');
            el.status.textContent = 'MOTION DETECTED';
        } else if (status === 'CLEAR') {
            el.panel.classList.add('is-clear');
            el.status.textContent = 'CLEAR';
        } else {
            el.panel.classList.add('is-nodata');
            el.status.textContent = 'NO DATA YET';
        }
    }

    // Colour each room card from that room's own status.
    function drawZones(zones) {
        if (!zones) { return; }

        Object.keys(zoneCards).forEach(function (code) {
            var card = zoneCards[code];
            var info = zones[code];
            if (!info) { return; }

            var isMotion = info.status === 'MOTION';

            card.className = 'zone-card ' + (isMotion ? 'zone-motion' : 'zone-none');

            card.querySelector('.zone-status').textContent =
                isMotion ? 'MOTION' : (info.status === 'CLEAR' ? 'Clear' : 'No data yet');

            card.querySelector('.zone-last').textContent =
                info.last_motion ? info.last_motion : 'never';

            card.querySelector('.zone-sms').textContent =
                info.last_sms ? info.last_sms : 'none yet';
        });
    }

    function drawSmsTable(rows) {
        if (!rows || rows.length === 0) {
            emptyRow(el.smsBody, 6, 'No SMS alerts recorded yet.');
            return;
        }

        var html = '';
        rows.forEach(function (row) {
            var when  = splitStamp(row.sent_at);
            var cls   = 'sms-status-' + String(row.status || '').toLowerCase();

            html += '<tr>'
                 +  '<td>' + safe(row.id) + '</td>'
                 +  '<td><span class="pill ' + cls + '">' + safe(row.status) + '</span></td>'
                 +  '<td>' + safe(row.zone) + '</td>'
                 +  '<td>' + (row.detail ? safe(row.detail) : '—') + '</td>'
                 +  '<td>' + safe(when.date) + '</td>'
                 +  '<td>' + safe(when.time) + '</td>'
                 +  '</tr>';
        });

        el.smsBody.innerHTML = html;
    }

    function drawHistoryTable(rows) {
        if (!rows || rows.length === 0) {
            emptyRow(el.history, 6, 'No motion events yet. Wave at one of the sensors!');
            return;
        }

        var html = '';
        rows.forEach(function (row) {
            var when = splitStamp(row.detected_at);
            var cls  = row.event_type === 'MOTION_DETECTED' ? 'evt-detected' : 'evt-stopped';

            html += '<tr>'
                 +  '<td>' + safe(row.id) + '</td>'
                 +  '<td class="' + cls + '">' + safe(row.event_type) + '</td>'
                 +  '<td>' + safe(row.zone) + '</td>'
                 +  '<td>' + safe(row.source) + '</td>'
                 +  '<td>' + safe(when.date) + '</td>'
                 +  '<td>' + safe(when.time) + '</td>'
                 +  '</tr>';
        });

        el.history.innerHTML = html;
    }

    function draw(data) {
        drawStatus(data.status);
        drawZones(data.zones);

        el.total.textContent = data.stats.total_events;
        el.today.textContent = data.stats.today_events;
        el.sms.textContent   = data.stats.sms_today;
        el.last.textContent  = data.stats.last_motion ? data.stats.last_motion : 'Never';

        if (data.sms.last) {
            var when = splitStamp(data.sms.last.sent_at);
            el.smsLast.textContent = data.sms.last.status + ' at ' + when.time + ' on ' + when.date;
        } else {
            el.smsLast.textContent = 'none yet';
        }

        drawSmsTable(data.sms.recent);
        drawHistoryTable(data.motion.recent);

        el.updated.textContent = data.server_time;
    }


    // --------------------------------------------------------
    // Fetching
    // --------------------------------------------------------

    function refresh() {
        // The ?t= is a cache-buster: without it some browsers keep
        // handing back the very first answer they saw.
        fetch(API + '?t=' + Date.now(), { cache: 'no-store' })
            .then(function (response) { return response.json(); })
            .then(function (data) {
                if (!data.success) {
                    throw new Error(data.message || 'API said no');
                }
                el.offline.hidden = true;
                draw(data);
            })
            .catch(function (err) {
                // Show the strip but keep the last known numbers on
                // screen, and keep trying - Apache may just be restarting.
                el.offline.hidden = false;
                el.updated.textContent = 'failed';
                if (window.console) { console.warn('[pir_sms_test] refresh failed:', err); }
            });
    }

    refresh();                        // draw immediately on load
    setInterval(refresh, REFRESH_MS); // then keep it fresh
}());
