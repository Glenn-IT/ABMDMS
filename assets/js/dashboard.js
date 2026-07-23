/* ============================================================
   ABMDMS - Arduino Based Motion Detection Monitoring System
   File: assets/js/dashboard.js
   ============================================================

   WHAT THIS DOES
   --------------
   Every few seconds this script asks the PHP API for fresh
   data and updates the numbers on the page.

   The page is NEVER reloaded - only the parts that changed
   are redrawn. That is what makes the dashboard feel "live".
   ============================================================ */

(function () {
    'use strict';

    // --------------------------------------------------------
    // SETTINGS
    // --------------------------------------------------------

    var API_URL       = 'api/get_motion_logs.php';
    var REFRESH_MS    = 3000;   // how often to check for new data (3 seconds)

    // --------------------------------------------------------
    // GRAB THE PAGE ELEMENTS ONCE (faster than looking them up
    // again every single refresh)
    // --------------------------------------------------------

    var el = {
        statusPanel: document.getElementById('status-panel'),
        statusValue: document.getElementById('status-value'),
        lastUpdated: document.getElementById('last-updated'),
        badge:       document.getElementById('connection-badge'),

        cardStatus:  document.getElementById('card-status'),
        cardTotal:   document.getElementById('card-total'),
        cardToday:   document.getElementById('card-today'),
        cardLast:    document.getElementById('card-last'),

        historyBody: document.getElementById('history-body'),
        recordCount: document.getElementById('record-count'),

        prevBtn:     document.getElementById('prev-page'),
        nextBtn:     document.getElementById('next-page'),
        pageInfo:    document.getElementById('page-info')
    };

    // --------------------------------------------------------
    // MEMORY
    // --------------------------------------------------------

    var currentPage  = 1;      // which page of history we are viewing
    var totalPages   = 1;
    var isLoading    = false;  // stops two requests overlapping
    var lastTopId    = null;   // used to flash newly arrived rows


    // --------------------------------------------------------
    // HELPER: make text safe before putting it on the page.
    // This prevents anything stored in the database from being
    // treated as HTML code.
    // --------------------------------------------------------
    function escapeHtml(value) {
        return String(value)
            .replace(/&/g, '&amp;')
            .replace(/</g, '&lt;')
            .replace(/>/g, '&gt;')
            .replace(/"/g, '&quot;');
    }


    // --------------------------------------------------------
    // HELPER: show whether the dashboard is talking to the server
    // --------------------------------------------------------
    function setConnected(ok) {
        if (!el.badge) {
            return;
        }
        if (ok) {
            el.badge.textContent = 'Live';
            el.badge.className   = 'badge badge-live';
        } else {
            el.badge.textContent = 'Offline';
            el.badge.className   = 'badge badge-offline';
        }
    }


    // --------------------------------------------------------
    // STEP 1 - ASK THE SERVER FOR FRESH DATA
    // --------------------------------------------------------
    function loadData() {

        // If the previous request has not finished, skip this turn.
        if (isLoading) {
            return;
        }
        isLoading = true;

        fetch(API_URL + '?page=' + currentPage, { cache: 'no-store' })

            .then(function (response) {
                if (!response.ok) {
                    throw new Error('Server returned ' + response.status);
                }
                return response.json();
            })

            .then(function (data) {
                if (!data.success) {
                    throw new Error(data.message || 'API error');
                }
                setConnected(true);
                render(data);
            })

            .catch(function (error) {
                // Something went wrong (Apache stopped, MySQL stopped,
                // wrong URL...). Show it instead of freezing silently.
                setConnected(false);
                console.error('ABMDMS dashboard:', error.message);
            })

            .finally(function () {
                isLoading = false;
            });
    }


    // --------------------------------------------------------
    // STEP 2 - PUT THE DATA ON THE PAGE
    // --------------------------------------------------------
    function render(data) {

        // ---- 2a. The big status panel ----
        var isMotion = (data.status === 'MOTION');

        el.statusValue.textContent = isMotion ? 'MOTION DETECTED' : 'NO MOTION';
        el.statusPanel.className   = 'status-panel ' + (isMotion ? 'status-motion' : 'status-none');

        // ---- 2b. The four summary cards ----
        el.cardStatus.textContent = isMotion ? 'MOTION DETECTED' : 'NO MOTION';
        el.cardStatus.className   = 'card-value card-value-small ' + (isMotion ? 'is-motion' : 'is-clear');

        el.cardTotal.textContent = data.total_events;
        el.cardToday.textContent = data.today_events;
        el.cardLast.textContent  = data.last_motion;

        el.lastUpdated.textContent = data.server_time;

        // ---- 2c. The history table ----
        renderTable(data.logs);

        // ---- 2d. Record count + pagination ----
        el.recordCount.textContent = data.total_rows + (data.total_rows === 1 ? ' record' : ' records');

        currentPage = data.page;
        totalPages  = data.total_pages;

        el.pageInfo.textContent  = 'Page ' + currentPage + ' of ' + totalPages;
        el.prevBtn.disabled      = (currentPage <= 1);
        el.nextBtn.disabled      = (currentPage >= totalPages);
    }


    // --------------------------------------------------------
    // STEP 3 - BUILD THE TABLE ROWS
    // --------------------------------------------------------
    function renderTable(logs) {

        // Nothing recorded yet
        if (!logs || logs.length === 0) {
            el.historyBody.innerHTML =
                '<tr><td colspan="5" class="empty">' +
                'No motion events recorded yet.<br>' +
                'Wave your hand in front of the PIR sensor, or use the Test Tool.' +
                '</td></tr>';
            lastTopId = null;
            return;
        }

        var newestId = logs[0].id;
        var rows     = '';

        for (var i = 0; i < logs.length; i++) {
            var log = logs[i];

            // Highlight rows that arrived since the last refresh
            var isNew = (lastTopId !== null && log.id > lastTopId && currentPage === 1);

            var pillClass = (log.event_type === 'MOTION_DETECTED')
                ? 'pill pill-detected'
                : 'pill pill-stopped';

            rows += '<tr' + (isNew ? ' class="is-new"' : '') + '>' +
                        '<td>' + log.id + '</td>' +
                        '<td><span class="' + pillClass + '">' + escapeHtml(log.event_type) + '</span></td>' +
                        '<td>' + escapeHtml(log.source) + '</td>' +
                        '<td>' + escapeHtml(log.date) + '</td>' +
                        '<td>' + escapeHtml(log.time) + '</td>' +
                    '</tr>';
        }

        el.historyBody.innerHTML = rows;

        // Remember the newest id so next time we know what is new
        if (currentPage === 1) {
            lastTopId = newestId;
        }
    }


    // --------------------------------------------------------
    // STEP 4 - PAGINATION BUTTONS
    // --------------------------------------------------------

    el.prevBtn.addEventListener('click', function () {
        if (currentPage > 1) {
            currentPage--;
            lastTopId = null;   // do not flash rows when paging
            loadData();
        }
    });

    el.nextBtn.addEventListener('click', function () {
        if (currentPage < totalPages) {
            currentPage++;
            lastTopId = null;
            loadData();
        }
    });


    // --------------------------------------------------------
    // STEP 5 - START
    // --------------------------------------------------------

    loadData();                          // load immediately
    setInterval(loadData, REFRESH_MS);   // then every few seconds

})();
