# Arduino → PHP → MySQL → Live Dashboard: Reusable Build Guide

A proven, working pattern for connecting **any Arduino sensor** to a **PHP/MySQL web system on XAMPP (Windows)**.

This was extracted from a finished, working project (Arduino PIR motion monitoring). Every code block here is
based on code that actually runs — not theory. Copy this file into a new project as `.md`, fill in the
placeholders, and follow the build order.

---

## PART 0 — Paste this to your AI assistant

> Copy the block below into your other project's chat to get the same system built there.

```text
I have a PHP/MySQL web system running on XAMPP (Windows) that is already finished.
I now need to connect an Arduino to it, following the architecture in
ARDUINO_WEB_INTEGRATION_GUIDE.md (attached / in this repo).

My hardware:
- Board:            [e.g. Arduino Uno]
- Sensor/module:    [e.g. RC522 RFID reader / HC-SR04 ultrasonic / DHT11 / load cell]
- What it measures: [e.g. a student ID card tap]

What I want recorded:
- Event/reading names the Arduino should print: [e.g. CARD:0A1B2C3D]
- Table it should be saved into:                [e.g. attendance_logs]
- What the dashboard should show:               [e.g. today's taps, live status]

Build it in the order given in the guide (Arduino → DB → API → bridge → dashboard),
and give me a test step after each layer so I can confirm it works before moving on.
Use the PowerShell serial bridge — do NOT use PHP fopen() on the COM port.
```

---

## PART 1 — The architecture (understand this first)

```
┌──────────┐   USB      ┌─────────────────┐   HTTP POST   ┌──────────┐   SQL   ┌────────┐
│ Arduino  │ ─────────► │ PowerShell      │ ────────────► │ PHP API  │ ──────► │ MySQL  │
│ + sensor │  serial    │ serial bridge   │   localhost   │ (insert) │  PDO    │        │
└──────────┘  9600 baud └─────────────────┘               └──────────┘         └───┬────┘
                                                                                   │
                                                          ┌──────────┐   HTTP GET  │
                                                          │ Browser  │ ◄───────────┘
                                                          │dashboard │  polls PHP API
                                                          └──────────┘  every 3 sec
```

**Five layers, each independently testable.** That is the whole point of this design — when something
breaks you can tell *exactly* which layer failed instead of guessing.

| Layer | Component | Test it alone with |
|-------|-----------|--------------------|
| 1 | Arduino sketch | Arduino IDE Serial Monitor |
| 2 | Database | phpMyAdmin |
| 3 | PHP API | A browser test page / simulator script |
| 4 | Serial bridge | Its own console output |
| 5 | Dashboard | Browser + simulator |

### The single most important rule

> **The Arduino never talks to MySQL. The browser never talks to the Arduino.**
> Everything goes through the PHP API. That one rule is what makes each layer swappable and testable.

---

## PART 2 — Non-negotiable lessons (read before coding)

These cost real debugging hours. Respect them and you skip the pain.

1. **Do NOT use PHP `fopen('COM5:', ...)` to read the serial port on Windows.**
   It fails even when the port is completely free (confirmed on PHP 8.2 ZTS). It is a PHP limitation, not
   a wiring problem. **Use the PowerShell `.NET SerialPort` bridge** in Part 6. PowerShell ships with every
   Windows PC — nothing to install. (Python + pyserial is the other good option.)

2. **Only ONE program may hold a COM port at a time.** Arduino IDE 2.x keeps the port open in the
   background even after you close the Serial Monitor *tab*. Close the **entire IDE** before running the bridge.

3. **Print only on state CHANGE, never every loop.** A sensor read in `loop()` runs ~20–1000×/second.
   If you print every pass you flood the database with thousands of junk rows per minute. Track the last
   state in a variable and print only when it flips.

4. **Whitelist your event types in PHP.** The API must refuse anything not in an allowed list. This is your
   main protection against junk and injected data reaching the DB.

5. **Add a duplicate-guard in the bridge.** Even with rule 3, electrical noise can double-fire. Ignore an
   identical event repeated within N seconds.

6. **Timestamp in PHP, not on the Arduino.** The Arduino has no real clock. Let PHP write `detected_at`
   using `date('Y-m-d H:i:s')` with a `date_default_timezone_set()` configured.

7. **Build a simulator early.** A small script that POSTs fake events to the API lets you finish and demo
   the entire web half *without the hardware plugged in*.

---

## PART 3 — File structure to create

Add these to your existing project. Nothing here disturbs your current pages.

```
your-project/
├── config.php                      ← settings + allowed event whitelist
├── database.php                    ← PDO connection helper
├── database/
│   └── database.sql                ← table definition (import in phpMyAdmin)
├── api/
│   ├── record_event.php            ← POST: bridge writes events here
│   └── get_events.php              ← GET : dashboard reads from here
├── serial/
│   ├── serial_reader.ps1           ← THE BRIDGE (PowerShell)
│   ├── start_reader.bat            ← double-click launcher
│   └── list_ports.bat              ← finds your COM port
├── arduino/
│   └── your_sketch/
│       └── your_sketch.ino
├── assets/js/dashboard.js          ← polling + live DOM updates
└── tools/simulate_event.php        ← test the system with no hardware
```

**Placeholders used below** — replace consistently everywhere:

| Placeholder | Meaning | Example |
|-------------|---------|---------|
| `{{PROJECT}}` | Project folder name under `htdocs` | `ABMDMS` |
| `{{DB_NAME}}` | Database name | `motion_monitoring` |
| `{{TABLE}}` | Table name | `motion_logs` |
| `{{EVENT_A}}` / `{{EVENT_B}}` | Event strings the Arduino prints | `MOTION_DETECTED` / `MOTION_STOPPED` |
| `{{SOURCE}}` | Label of the hardware source | `ARDUINO_PIR` |
| `{{COM}}` | Serial port | `COM5` |

---

## PART 4 — Build order (do NOT skip ahead)

Each step ends with a checkpoint. Do not start the next step until the checkpoint passes.

### Step 1 — Arduino sketch

**Template — state-change reporting (the pattern that matters):**

```cpp
const int SENSOR_PIN = 2;
const int LED_PIN    = 13;              // built-in LED = free visual debug
const unsigned long BAUD_RATE       = 9600;
const unsigned long WARMUP_SECONDS  = 30;   // only if your sensor needs it (PIR does)
const unsigned long CONFIRM_MS      = 2000; // debounce before declaring "ended"

bool eventActive  = false;    // remembers what we already reported
unsigned long lowStartedAt = 0;

void setup() {
  Serial.begin(BAUD_RATE);
  pinMode(SENSOR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  // Warm-up countdown so the user knows it isn't frozen
  for (unsigned long i = WARMUP_SECONDS; i > 0; i--) {
    Serial.print(i); Serial.println("...");
    delay(1000);
  }
  Serial.println("System Ready");
}

void loop() {
  int value = digitalRead(SENSOR_PIN);

  if (value == HIGH) {
    lowStartedAt = 0;
    if (!eventActive) {                 // ← only on CHANGE
      eventActive = true;
      digitalWrite(LED_PIN, HIGH);
      Serial.println("{{EVENT_A}}");    // ← the line the bridge reads
    }
  } else {
    if (eventActive) {
      if (lowStartedAt == 0) lowStartedAt = millis();
      if (millis() - lowStartedAt >= CONFIRM_MS) {   // ← debounce
        eventActive = false;
        digitalWrite(LED_PIN, LOW);
        Serial.println("{{EVENT_B}}");
      }
    }
  }
  delay(50);                            // don't hammer the pin
}
```

**Design rules for the sketch:**
- Print **machine-readable, ALL_CAPS, no-spaces** tokens (`{{EVENT_A}}`), one per line, `Serial.println()`.
- Human messages ("Warming up...", "System Ready") are fine — the bridge ignores anything not whitelisted.
- For value readings use a `KEY:VALUE` format, e.g. `TEMP:27.4`, `CARD:0A1B2C3D`, `WEIGHT:1250`.
- Use the built-in LED on pin 13 as a free visual confirmation.

**Checkpoint 1:** Upload (wait for **"Done uploading."** — the memory-usage message alone means only
*compiled*, not uploaded). Open Serial Monitor at 9600. Trigger the sensor. You should see clean event
lines, and **silence when nothing is happening**. Do not continue until it is quiet at rest.

---

### Step 2 — Database

```sql
CREATE DATABASE IF NOT EXISTS `{{DB_NAME}}`
    DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci;

USE `{{DB_NAME}}`;

CREATE TABLE IF NOT EXISTS `{{TABLE}}` (
    `id`          INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `event_type`  VARCHAR(20)  NOT NULL,                          -- {{EVENT_A}} / {{EVENT_B}}
    `source`      VARCHAR(50)  NOT NULL DEFAULT '{{SOURCE}}',     -- hardware or SIMULATOR
    `detected_at` DATETIME     NOT NULL,                          -- when it happened (PHP clock)
    `created_at`  TIMESTAMP    NOT NULL DEFAULT CURRENT_TIMESTAMP,-- when the row was written
    PRIMARY KEY (`id`),
    KEY `idx_detected_at` (`detected_at`),
    KEY `idx_event_type`  (`event_type`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;
```

Import via **phpMyAdmin → Import**. Add a `value DECIMAL(10,2)` column if your sensor reports numbers.

**Checkpoint 2:** Table visible in phpMyAdmin.

---

### Step 3 — Config + DB connection

`config.php` — the **only** file anyone should need to edit:

```php
<?php
define('DB_HOST', 'localhost');
define('DB_PORT', '3306');
define('DB_NAME', '{{DB_NAME}}');
define('DB_USER', 'root');          // XAMPP default
define('DB_PASS', '');              // XAMPP default = empty
define('DB_CHARSET', 'utf8mb4');

date_default_timezone_set('Asia/Manila');   // your timezone

// SECURITY: the API refuses to save anything not in this list
define('ALLOWED_EVENT_TYPES', ['{{EVENT_A}}', '{{EVENT_B}}']);

define('RECORDS_PER_PAGE', 20);
define('MAX_RECORDS_PER_PAGE', 100);

define('SHOW_ERRORS', true);        // set false before the demo
if (SHOW_ERRORS) { ini_set('display_errors','1'); error_reporting(E_ALL); }
else             { ini_set('display_errors','0'); error_reporting(0); }
```

`database.php` — PDO with a reused connection:

```php
<?php
require_once __DIR__ . '/config.php';

function getDB(): PDO {
    static $pdo = null;                 // reuse one connection per request
    if ($pdo !== null) return $pdo;

    $dsn = 'mysql:host='.DB_HOST.';port='.DB_PORT.';dbname='.DB_NAME.';charset='.DB_CHARSET;

    $pdo = new PDO($dsn, DB_USER, DB_PASS, [
        PDO::ATTR_ERRMODE            => PDO::ERRMODE_EXCEPTION,
        PDO::ATTR_DEFAULT_FETCH_MODE => PDO::FETCH_ASSOC,
        PDO::ATTR_EMULATE_PREPARES   => false,   // real prepared statements
    ]);
    return $pdo;
}

function testDBConnection(): array {
    try {
        getDB()->query('SELECT 1');
        return ['ok' => true, 'message' => 'Database connected.'];
    } catch (PDOException $e) {
        error_log('DB error: ' . $e->getMessage());          // detail → log
        return ['ok' => false, 'message' => 'Cannot connect to MySQL. Is it running in XAMPP?'];
    }
}
```

---

### Step 4 — The write API (`api/record_event.php`)

This is the mailbox the bridge posts to.

```php
<?php
require_once __DIR__ . '/../database.php';
header('Content-Type: application/json; charset=utf-8');

function respond(bool $ok, string $msg, array $extra = [], int $code = 200): void {
    http_response_code($code);
    echo json_encode(array_merge(['success' => $ok, 'message' => $msg], $extra));
    exit;
}

// 1. POST only — writing must never happen from a plain browser visit
if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    respond(false, 'This endpoint only accepts POST requests.', [], 405);
}

// 2. Read input
$eventType = isset($_POST['event_type']) ? trim((string)$_POST['event_type']) : '';
$source    = isset($_POST['source'])     ? trim((string)$_POST['source'])     : '{{SOURCE}}';

// 3. Validate  ← the security gate
if ($eventType === '') {
    respond(false, 'event_type is missing.', [], 400);
}
$eventType = strtoupper($eventType);
if (!in_array($eventType, ALLOWED_EVENT_TYPES, true)) {
    respond(false, 'Invalid event_type.', [], 400);
}
if ($source === '') $source = '{{SOURCE}}';
$source = strtoupper(substr(preg_replace('/[^A-Za-z0-9_\- ]/', '', $source), 0, 50));

// 4. Save with a prepared statement
try {
    $db = getDB();
    $detectedAt = date('Y-m-d H:i:s');          // PHP owns the clock

    $stmt = $db->prepare(
        'INSERT INTO {{TABLE}} (event_type, source, detected_at) VALUES (?, ?, ?)'
    );
    $stmt->execute([$eventType, $source, $detectedAt]);

    respond(true, 'Event recorded successfully', [
        'id'          => (int)$db->lastInsertId(),
        'event_type'  => $eventType,
        'source'      => $source,
        'detected_at' => $detectedAt,
    ]);
} catch (PDOException $e) {
    error_log('record_event error: ' . $e->getMessage());   // real reason → log
    respond(false, 'Unable to record event', [], 500);      // generic → caller
}
```

**Why it returns JSON with `success` + `id`:** the bridge prints `saved as record #12` in its console, so you
get instant end-to-end confirmation without opening phpMyAdmin.

**Checkpoint 4 — test with no hardware.** Create `tools/simulate_event.php` that POSTs `{{EVENT_A}}` with
`source=SIMULATOR` (or use PowerShell):

```powershell
Invoke-RestMethod -Uri "http://localhost/{{PROJECT}}/api/record_event.php" -Method Post `
  -Body @{ event_type = "{{EVENT_A}}"; source = "SIMULATOR" }
```

Expect `success: True` and a new row in phpMyAdmin.

---

### Step 5 — The read API (`api/get_events.php`)

One request returns **everything** the dashboard needs — status, stats, and the table page.

```php
<?php
require_once __DIR__ . '/../database.php';
header('Content-Type: application/json; charset=utf-8');
header('Cache-Control: no-store, no-cache, must-revalidate');   // never serve stale data

try {
    $db = getDB();

    // --- pagination (cast to int so ?page=abc can't break SQL) ---
    $page  = isset($_GET['page'])  ? (int)$_GET['page']  : 1;   if ($page  < 1) $page  = 1;
    $limit = isset($_GET['limit']) ? (int)$_GET['limit'] : RECORDS_PER_PAGE;
    if ($limit < 1) $limit = RECORDS_PER_PAGE;
    if ($limit > MAX_RECORDS_PER_PAGE) $limit = MAX_RECORDS_PER_PAGE;

    $totalRows  = (int)$db->query('SELECT COUNT(*) FROM {{TABLE}}')->fetchColumn();
    $totalPages = $totalRows > 0 ? (int)ceil($totalRows / $limit) : 1;
    if ($page > $totalPages) $page = $totalPages;
    $offset = ($page - 1) * $limit;

    // --- stat cards ---
    $stmt = $db->prepare('SELECT COUNT(*) FROM {{TABLE}} WHERE event_type = ?');
    $stmt->execute(['{{EVENT_A}}']);
    $totalEvents = (int)$stmt->fetchColumn();

    $stmt = $db->prepare('SELECT COUNT(*) FROM {{TABLE}} WHERE event_type = ? AND DATE(detected_at) = ?');
    $stmt->execute(['{{EVENT_A}}', date('Y-m-d')]);
    $todayEvents = (int)$stmt->fetchColumn();

    // --- live status: what was the newest event of ANY kind? ---
    $newest = $db->query('SELECT event_type FROM {{TABLE}} ORDER BY id DESC LIMIT 1')->fetchColumn();
    $status = ($newest === '{{EVENT_A}}') ? 'ACTIVE' : 'IDLE';

    // --- history rows, newest first ---
    $stmt = $db->prepare('SELECT id, event_type, source, detected_at FROM {{TABLE}}
                          ORDER BY id DESC LIMIT :limit OFFSET :offset');
    $stmt->bindValue(':limit',  $limit,  PDO::PARAM_INT);   // MUST bind as int
    $stmt->bindValue(':offset', $offset, PDO::PARAM_INT);
    $stmt->execute();

    $logs = [];
    foreach ($stmt->fetchAll() as $row) {
        $t = strtotime($row['detected_at']);
        $logs[] = [
            'id'         => (int)$row['id'],
            'event_type' => $row['event_type'],
            'source'     => $row['source'],
            'date'       => date('M j, Y', $t),
            'time'       => date('g:i:s A', $t),   // format server-side, not in JS
        ];
    }

    echo json_encode([
        'success'      => true,
        'status'       => $status,
        'total_events' => $totalEvents,
        'today_events' => $todayEvents,
        'logs'         => $logs,
        'page'         => $page,
        'total_pages'  => $totalPages,
        'total_rows'   => $totalRows,
        'server_time'  => date('g:i:s A'),
    ]);
} catch (PDOException $e) {
    error_log('get_events error: ' . $e->getMessage());
    http_response_code(500);
    echo json_encode(['success' => false, 'message' => 'Unable to load events.']);
}
```

**Two gotchas baked in above:**
- `LIMIT`/`OFFSET` must use `bindValue(..., PDO::PARAM_INT)` — string-bound values produce a SQL syntax error.
- Format dates/times in **PHP**, not JavaScript. Keeps the timezone consistent and the JS trivial.

---

### Step 6 — The serial bridge (the critical piece)

`serial/serial_reader.ps1` — **use PowerShell, not PHP.**

```powershell
# ===== SETTINGS =====
$ComPort  = "{{COM}}"
$BaudRate = 9600                                                   # must match Serial.begin()
$ApiUrl   = "http://localhost/{{PROJECT}}/api/record_event.php"
$Source   = "{{SOURCE}}"
$AllowedEvents   = @("{{EVENT_A}}", "{{EVENT_B}}")
$DuplicateWindow = 2      # seconds — ignore identical repeats
$ReconnectDelay  = 3      # seconds — retry after a lost port

function Say([string]$t) { Write-Host ("[" + (Get-Date -Format "HH:mm:ss") + "] " + $t) }

$lastEvent = ""
$lastTime  = Get-Date "2000-01-01"

while ($true) {                                    # OUTER loop = auto-reconnect
    $port = $null
    try {
        $port = New-Object System.IO.Ports.SerialPort $ComPort, $BaudRate,
                ([System.IO.Ports.Parity]::None), 8, ([System.IO.Ports.StopBits]::One)
        $port.ReadTimeout = 500                    # ms — also lets Ctrl+C work
        $port.NewLine     = "`n"
        $port.Open()
    } catch {
        Say ("Could not open " + $ComPort + ": " + $_.Exception.Message)
        Say "  - Arduino plugged in? Serial Monitor CLOSED? Correct COM port?"
        Start-Sleep -Seconds $ReconnectDelay
        continue
    }

    Say ("Connected to " + $ComPort + ". Waiting for events...")

    while ($port.IsOpen) {                         # INNER loop = read lines
        try { $line = $port.ReadLine() }
        catch [System.TimeoutException] { continue }    # no data yet — normal
        catch { break }                                 # port lost → reconnect

        $msg = $line.Trim()
        if ($msg -eq "") { continue }

        Write-Host ("    Arduino: " + $msg)         # echo EVERYTHING (great for debugging)

        if ($AllowedEvents -notcontains $msg) { continue }   # ignore chatter

        # duplicate guard
        $now = Get-Date
        if ($msg -eq $lastEvent -and ($now - $lastTime).TotalSeconds -lt $DuplicateWindow) {
            Say ("Skipped duplicate " + $msg); continue
        }
        $lastEvent = $msg; $lastTime = $now

        # POST to the API
        try {
            $r = Invoke-RestMethod -Uri $ApiUrl -Method Post -TimeoutSec 5 `
                 -Body @{ event_type = $msg; source = $Source }
            if ($r.success) { Say ("[OK]   " + $msg + " -> record #" + $r.id) }
            else            { Say ("[FAIL] " + $msg + " -> " + $r.message) }
        } catch {
            Say ("[FAIL] " + $msg + " -> Cannot reach API. Is Apache running?")
        }
    }

    if ($port -ne $null -and $port.IsOpen) { $port.Close() }
    Say ("Lost connection. Reconnecting in " + $ReconnectDelay + "s...")
    Start-Sleep -Seconds $ReconnectDelay
}
```

**Why each piece exists:**

| Piece | Reason |
|-------|--------|
| Outer `while` loop | Arduino unplugged / port dropped → auto-reconnects instead of dying |
| `ReadTimeout = 500` | Without it, `ReadLine()` blocks forever and `Ctrl+C` won't work |
| `catch [TimeoutException] { continue }` | "No data yet" is normal, not an error |
| `Write-Host "Arduino: $msg"` | Echoing raw lines shows warm-up/boot text — invaluable when debugging |
| Duplicate window | Second line of defence against noise double-firing |
| `-TimeoutSec 5` | A hung Apache can't freeze the reader |

`serial/start_reader.bat` — double-click launcher:

```bat
@echo off
title Serial Reader
color 0B
cd /d "%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0serial_reader.ps1"
echo.
echo The serial reader has stopped.
pause
```

`-ExecutionPolicy Bypass` matters — without it Windows blocks the `.ps1` on most machines.

`serial/list_ports.bat` — find your COM port:

```bat
@echo off
title COM Port Finder
echo   COM PORTS ON THIS COMPUTER
echo.
reg query "HKLM\HARDWARE\DEVICEMAP\SERIALCOMM" 2>nul
echo.
echo   TIP: run with Arduino UNPLUGGED, then plug it in and run again.
echo   The port that APPEARED is your Arduino.
pause
```

**Checkpoint 6:** Close the Arduino IDE completely → double-click `start_reader.bat` → trigger the sensor.
You should see `Arduino: {{EVENT_A}}` followed by `[OK] {{EVENT_A}} -> record #1`.

---

### Step 7 — Live dashboard (polling)

No WebSockets needed. Poll the read API every 3 seconds and update only the changed DOM nodes.

```javascript
(function () {
    'use strict';

    var API_URL    = 'api/get_events.php';
    var REFRESH_MS = 3000;

    var el = {                                   // cache elements once
        statusValue: document.getElementById('status-value'),
        cardTotal:   document.getElementById('card-total'),
        cardToday:   document.getElementById('card-today'),
        historyBody: document.getElementById('history-body'),
        badge:       document.getElementById('connection-badge')
    };

    var currentPage = 1, isLoading = false;

    function escapeHtml(v) {                     // ALWAYS escape DB values
        return String(v).replace(/&/g,'&amp;').replace(/</g,'&lt;')
                        .replace(/>/g,'&gt;').replace(/"/g,'&quot;');
    }

    function refresh() {
        if (isLoading) return;                   // stop overlapping requests
        isLoading = true;

        fetch(API_URL + '?page=' + currentPage)
            .then(function (r) { return r.json(); })
            .then(function (data) {
                if (!data.success) return;
                el.badge.textContent = 'Connected';
                el.statusValue.textContent = data.status;
                el.cardTotal.textContent   = data.total_events;
                el.cardToday.textContent   = data.today_events;

                var html = '';
                data.logs.forEach(function (log) {
                    html += '<tr><td>' + log.id + '</td>'
                          + '<td>' + escapeHtml(log.event_type) + '</td>'
                          + '<td>' + escapeHtml(log.source) + '</td>'
                          + '<td>' + escapeHtml(log.date) + '</td>'
                          + '<td>' + escapeHtml(log.time) + '</td></tr>';
                });
                el.historyBody.innerHTML = html;
            })
            .catch(function () { el.badge.textContent = 'Disconnected'; })
            .finally(function () { isLoading = false; });
    }

    refresh();
    setInterval(refresh, REFRESH_MS);
})();
```

**Rules:** never reload the whole page; guard against overlapping requests with `isLoading`;
always `escapeHtml()` anything from the database; show a connection badge so a dead Apache is obvious.

---

## PART 5 — Adapting to other sensors

The architecture never changes. Only the sketch and the allowed-event list do.

| Sensor | Arduino prints | Allowed list / schema change |
|--------|----------------|------------------------------|
| PIR motion (HC-SR501) | `MOTION_DETECTED` / `MOTION_STOPPED` | as-is |
| RFID (RC522) | `CARD:0A1B2C3D` | store the UID in a `card_uid` column; validate with `preg_match('/^[A-F0-9]{8}$/')` |
| Ultrasonic (HC-SR04) | `DISTANCE:24.5` | add `value DECIMAL(10,2)`; validate numeric range |
| Temp/humidity (DHT11) | `TEMP:27.4` / `HUM:65` | add `value`; **throttle to once per 30–60s**, not every loop |
| Load cell (HX711) | `WEIGHT:1250` | add `value`; report only on meaningful change |
| Button/switch | `BUTTON_PRESSED` | debounce ~50ms in the sketch |
| Flame/gas/smoke | `ALARM_ON` / `ALARM_OFF` | as-is; consider an alert on the dashboard |

**For `KEY:VALUE` sensors,** split in the bridge and send both fields:

```powershell
if ($msg -match '^([A-Z]+):(.+)$') {
    $body = @{ event_type = $Matches[1]; value = $Matches[2]; source = $Source }
    Invoke-RestMethod -Uri $ApiUrl -Method Post -Body $body
}
```

…and validate the value in PHP (`is_numeric()` + a sane min/max) before inserting.

**For continuous readings, throttle.** A temperature sensor does not need 20 rows per second — send at most
one every 30–60 seconds, or only when the value changes by more than a threshold.

---

## PART 6 — Troubleshooting table

| Symptom | Cause | Fix |
|---------|-------|-----|
| Sketch shows "uses 2802 bytes (8%)" and stops | You clicked **Verify** (✔), not **Upload** (→) | Click the arrow. **"Done uploading." is the only proof.** |
| Sensor fires while nothing moves | PIR needs a 30–60s warm-up **with nobody near it** | Reset, walk away, wait, then test. Also: lower the sensitivity pot, keep away from fans/windows/laptop heat, jumper on **H** |
| Fires no matter what — even covered | **Floating input pin** — the OUT wire isn't reaching the pin | Reseat wires; **read the labels printed under the sensor dome** (pin order varies per board!). Isolation test: jumper the input pin to GND — if it goes quiet, the code is fine and the wiring is at fault |
| "Could not open COM5 — in use" though Monitor is closed | Arduino IDE 2.x holds the port in the background | 1) Close the **entire IDE** 2) close old reader windows 3) unplug USB 10s, replug 4) Device Manager → Ports → Disable/Enable |
| Other tools see the port, PHP cannot open it | **PHP on Windows can't reliably open COM ports** | Use the PowerShell bridge. Not a wiring bug — don't chase it |
| `[FAIL] ... Cannot reach the API` | Apache not running, or wrong URL | Start Apache in XAMPP; open the API URL in a browser to confirm the path |
| Bridge says `[OK]` but the dashboard is empty | Dashboard is querying a different DB/table, or JS error | Check the API URL in the browser; open DevTools Console |
| Rows flooding in by the thousand | Printing every `loop()` instead of on state change | Add the `eventActive` flag pattern |
| TX light flashes / L light toggles | **Normal** — TX = serial send, L = pin-13 LED | Nothing to fix; it means it's working ✅ |
| `.ps1` won't run ("script is disabled") | Windows execution policy | Launch via the `.bat` with `-ExecutionPolicy Bypass` |

---

## PART 7 — Daily run order & demo checklist

**To run the system:**
1. XAMPP → start **Apache** + **MySQL** (both green)
2. Plug in the Arduino (the sketch already lives on the board — the IDE does **not** need to be open)
3. **Close the Arduino IDE entirely**
4. Double-click `serial/start_reader.bat` → wait for `Connected to {{COM}}`
5. Open `http://localhost/{{PROJECT}}/`
6. Trigger the sensor → bridge prints `[OK]` → dashboard updates within 3 seconds

**Before a demo/defence:**
- [ ] Set `SHOW_ERRORS` to `false` in `config.php`
- [ ] `TRUNCATE TABLE {{TABLE}};` for a clean start, or seed a few realistic rows
- [ ] Let the sensor finish warm-up **before** the audience is watching
- [ ] Keep `tools/simulate_event.php` ready as a fallback if the hardware misbehaves
- [ ] Confirm the COM port hasn't changed (a different USB slot = a different port number)

---

## PART 8 — Debugging philosophy (the meta-lesson)

1. **Isolate the layer.** Test sensor → API → DB → dashboard *separately*. When something breaks you know
   instantly which one failed. This is why the simulator and the bridge's console echo exist.
2. **"Works no matter what I do" = a floating input or a disconnected wire**, not a dead component.
3. **When one tool can't do a job, test whether another can.** PHP couldn't open the COM port; PowerShell
   opened it instantly — that single test found the real cause in seconds instead of hours.
4. **Change one thing at a time**, then re-test, so you know what actually fixed it.
5. **Read the labels on the actual hardware.** Pin order differs between boards; don't trust a tutorial photo.
6. **One program per COM port.** If it won't open, something already holds it.
