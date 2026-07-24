# ABMDMS → Android App Integration Guide

**Purpose of this file:** paste this into Claude Code inside the
`MBPSAASMobileBasedPoultrySecurityandAlertSystem` Android Studio project so it has full
context on the existing backend and can wire up the networking layer correctly.

This describes a **finished, working** system: Arduino (3-zone PIR motion sensor) → PowerShell
serial bridge → PHP API → MySQL, running on a Windows PC via XAMPP. The Android app does **not**
need to talk to the Arduino or MySQL directly — it only needs to call two existing PHP HTTP
endpoints, the same way a browser dashboard already does.

---

## 1. What already exists (do not rebuild this)

```
┌──────────┐   USB      ┌─────────────────┐   HTTP POST   ┌──────────┐   SQL   ┌────────┐
│ Arduino  │ ─────────► │ PowerShell      │ ────────────► │ PHP API  │ ──────► │ MySQL  │
│ 3x PIR   │  9600 baud │ serial bridge   │   localhost   │ (insert) │  PDO    │        │
└──────────┘            └─────────────────┘               └────┬─────┘         └───┬────┘
                                                                │                   │
                                                                │  HTTP GET         │
                          ┌──────────────────┐                 │                   │
                          │  Android App      │◄────────────────┘                   │
                          │  (this project)   │  polls PHP API, same data source ───┘
                          └──────────────────┘
```

- Project name: **ABMDMS** (Arduino Based Motion Detection Monitoring System)
- Runs on XAMPP (Apache + MySQL) on a Windows PC, at `http://localhost/ABMDMS/`
- Monitors **3 zones**: Room A, Room B, Room C — each with its own PIR sensor
- The Arduino, bridge, PHP, and MySQL layers are already built and working. **The Android app
  is a new, separate client that reads from the same PHP API the web dashboard uses.**

---

## 2. The API the Android app should call

Base URL when phone and PC are on the same Wi-Fi:
`http://<PC-LAN-IP>/ABMDMS/api/` — **not** `localhost` (that would mean the phone itself).
Find the PC's IP with `ipconfig` on the PC (look for the Wi-Fi adapter's IPv4 address).

### `GET api/get_motion_logs.php` — the only endpoint the Android app needs

Optional query params: `?page=1&limit=20`

**Response JSON (real shape, from the live code):**
```json
{
  "success": true,
  "status": "MOTION",
  "zones": {
    "ROOMA": { "label": "Room A", "status": "MOTION",    "last_motion": "July 24, 2026 12:30 PM" },
    "ROOMB": { "label": "Room B", "status": "NO_MOTION", "last_motion": "July 24, 2026 11:02 AM" },
    "ROOMC": { "label": "Room C", "status": "NO_MOTION", "last_motion": "No motion yet" }
  },
  "total_events": 125,
  "today_events": 25,
  "last_motion": "July 24, 2026 12:30 PM",
  "logs": [
    {
      "id": 1,
      "event_type": "MOTION_DETECTED",
      "zone": "ROOMA",
      "zone_label": "Room A",
      "source": "ARDUINO_PIR",
      "date": "Jul 24, 2026",
      "time": "12:30:05 PM"
    }
  ],
  "page": 1,
  "total_pages": 7,
  "total_rows": 125,
  "server_time": "12:31:40 PM"
}
```

- `status` / per-zone `status` is either `"MOTION"` or `"NO_MOTION"` — use this for a live
  alert/status screen.
- `zones` is keyed by zone code (`ROOMA`, `ROOMB`, `ROOMC`) — always exactly these three.
- `logs` is the history table, newest first, already paginated.
- Dates/times are pre-formatted strings from PHP — no timezone math needed on the Android side.
- On failure: `{"success": false, "message": "..."}` with HTTP 500.

### `POST api/record_motion.php` — the Android app normally does NOT call this

This is what the PowerShell bridge posts to when the Arduino detects motion. Fields:
`event_type` (`MOTION_DETECTED` / `MOTION_STOPPED`), `zone` (`ROOMA`/`ROOMB`/`ROOMC`),
`source` (defaults to `ARDUINO_PIR`). Only needed if you want the app itself to simulate/inject
a test event — otherwise leave this alone.

---

## 3. Database (for reference only — Android never touches this directly)

Table `motion_logs` in MySQL database `motion_monitoring`:

| Column | Type | Notes |
|---|---|---|
| id | INT UNSIGNED, PK, auto_increment | |
| event_type | VARCHAR(20) | `MOTION_DETECTED` or `MOTION_STOPPED` |
| zone | VARCHAR(20) | `ROOMA`, `ROOMB`, or `ROOMC` |
| source | VARCHAR(50) | `ARDUINO_PIR` or `SIMULATOR` |
| detected_at | DATETIME | when the motion happened (PHP clock, Asia/Manila) |
| created_at | TIMESTAMP | when the row was written |

---

## 4. What to build in the Android project

1. **Networking permission + cleartext traffic.** XAMPP serves plain HTTP, not HTTPS.
   Add `<uses-permission android:name="android.permission.INTERNET" />` in the manifest, and
   allow cleartext (either `android:usesCleartextTraffic="true"`, or better, a
   `network_security_config.xml` scoped to just the PC's LAN IP).
2. **Retrofit (or OkHttp) service interface** hitting `GET /api/get_motion_logs.php`.
3. **Data classes** matching the JSON shape above: a `MotionResponse` with `status`, `zones`
   (map of zone code → `ZoneStatus{label, status, last_motion}`), `totalEvents`, `todayEvents`,
   `lastMotion`, `logs` (list of `MotionLog`), `page`, `totalPages`.
4. **Polling.** Call the endpoint every few seconds (a `Handler`/coroutine loop while the
   relevant screen is visible, or `WorkManager` for background checks) — mirrors what the
   existing web dashboard does with `setInterval(refresh, 3000)`.
5. **UI:** a status screen showing overall + per-zone `MOTION` / `NO_MOTION`, and a history
   list backed by the `logs` array (supports pagination via `?page=`).
6. **Config:** make the base URL (PC's LAN IP) a single configurable value — it'll change if
   the PC moves to a different network or gets a new DHCP lease.

Nothing on the PHP/MySQL/Arduino/PowerShell-bridge side needs to change for any of this.

---

## 5. Known constraints / gotchas

- Phone and PC must be reachable from each other — same Wi-Fi network is simplest. `localhost`
  from the phone means the phone, not the PC.
- If the PC's IP changes (DHCP), the app's configured base URL needs updating too — consider
  making it editable in an in-app settings screen rather than hardcoding it.
- XAMPP's Apache + MySQL must both be running (green in XAMPP Control Panel) for the API to
  respond at all.
- `SHOW_ERRORS` in `config.php` should be set to `false` before any demo, but that only affects
  what PHP prints on error — it doesn't change the JSON contract above.
