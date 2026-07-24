# ABMDMS — Multi-Zone (3 PIR) Upgrade Checklist

Tracks the work to go from **1 PIR / 1 zone** to **3 PIR sensors / 3 zones**
(Room A = Pin 3, Room B = Pin 4, Room C = Pin 2, existing). Work through the
phases **in order** — each depends on the one before it.

See `arduino/PIR_MULTI_ZONE_WIRING.md` for the wiring diagram this checklist follows.

> Tip: in most Markdown editors you tick a box by changing `[ ]` to `[x]`.

---

## PHASE 1 — HARDWARE WIRING

- [x] Wire PIR #1 "Room C" (existing) — VCC/GND/OUT → 5V / GND / Pin 2
- [x] Wire PIR #2 "Room A" (new) — VCC/GND/OUT → breadboard rails / Pin 3
- [x] Wire PIR #3 "Room B" (new) — VCC/GND/OUT → breadboard rails / Pin 4
- [x] Set each PIR's yellow jumper to the **H** position
- [x] Turn each PIR's time-delay screw fully anti-clockwise
- [x] Double-check breadboard `(+)`/`(-)` rails aren't crossed or shorted
- [x] Connect Arduino Uno to laptop, confirm green power LED is ON

---

## PHASE 2 — VERIFY EACH SENSOR ALONE (Serial Monitor, no code changes yet)

- [x] Open Arduino IDE Serial Monitor at 9600 baud
- [x] Temporarily test Room A on Pin 3 wave-test with a throwaway sketch (or reuse existing single-pin sketch pointed at Pin 3)
- [x] Confirm Room A triggers cleanly and goes quiet at rest
- [x] Repeat the same isolated test for Room B on Pin 4
- [x] Confirm Room C on Pin 2 still works (unchanged from before)
- [x] Isolation-test any sensor that fires with nobody near it (OUT → GND jumper trick)

---

## PHASE 3 — UPDATE THE ARDUINO SKETCH

- [x] Open `arduino/motion_sensor/motion_sensor.ino`
- [x] Add pin constants for Room A (3) and Room B (4) alongside Room C (2)
- [x] Duplicate the state-change tracking (`motionActive`, `lowStartedAt`) per zone — 3 independent sets, not shared
- [x] Print per-zone event tokens, e.g. `ROOMA_MOTION_DETECTED` / `ROOMA_MOTION_STOPPED`, `ROOMB_...`, `ROOMC_...`
- [x] Keep the warm-up countdown, but confirm it covers all 3 sensors before "System Ready"
- [x] Upload and wait for **"Done uploading"**
- [x] Serial Monitor: confirm each zone reports independently and stays silent at rest
- [x] Confirm triggering one zone does **not** falsely trigger another

---

## PHASE 4 — DATABASE CHANGES

- [x] Open `database/database.sql`
- [x] Decide: add a `zone` column to `motion_logs` — the sketch prints combined tokens (`ROOMA_MOTION_DETECTED`), and the bridge will split them into `event_type` + `zone` before saving, so `event_type` stays just `MOTION_DETECTED`/`MOTION_STOPPED`
- [x] Added `zone VARCHAR(20) NOT NULL DEFAULT 'ROOMC'` to the `CREATE TABLE`, plus an `idx_zone` index, plus a commented `ALTER TABLE` upgrade path for an already-existing table
- [x] Re-import via phpMyAdmin (fresh install) **or** run the two commented `ALTER TABLE` lines in database.sql's "UPGRADE" section (existing table)
- [x] Confirm in phpMyAdmin that `motion_logs` now has a `zone` column

---

## PHASE 5 — UPDATE PHP CONFIG + APIs

- [x] `config.php`: `ALLOWED_EVENT_TYPES` stays `['MOTION_DETECTED', 'MOTION_STOPPED']` (unchanged) — added `ALLOWED_ZONES = ['ROOMA', 'ROOMB', 'ROOMC']` and `ZONE_LABELS` (friendly names) instead
- [x] Updated `api/record_motion.php` to accept and validate a `zone` field against `ALLOWED_ZONES`, and insert it into the new column
- [x] Updated `api/get_motion_logs.php` to return a `zones` object with per-zone status + last motion time, and added `zone`/`zone_label` to each history row
- [x] Tested with `Invoke-RestMethod` POSTs: valid zone (ROOMA) inserts correctly, invalid zone (ROOMX) is rejected with 400, and the read API returns correct per-zone status — test row cleaned up afterward

---

## PHASE 6 — UPDATE THE SERIAL BRIDGE

- [x] Updated both `serial/serial_reader.ps1` (the active bridge) and `serial/serial_reader.php` (kept in sync)
- [x] Split each raw line like `ROOMA_MOTION_DETECTED` into `zone=ROOMA` + `event_type=MOTION_DETECTED` (regex `^(ROOMA|ROOMB|ROOMC)_(MOTION_DETECTED|MOTION_STOPPED)$`)
- [x] Regex itself covers all 3 zones × 2 event types (6 combinations) — anything else is ignored as a status message
- [x] Duplicate-guard still keyed on the full raw line (which includes the zone), so it's per-zone by construction — Room A repeating never suppresses Room B
- [x] Both bridges now POST `event_type`, `zone`, and `source` to the API
- [x] Restarted `start_reader.bat` with all 3 sensors wired — confirmed it echoes all 3 zones' raw lines and shows `[OK]` for each

---

## PHASE 7 — UPDATE THE DASHBOARD

- [x] Added 3 per-zone status cards (Room A / Room B / Room C) to `index.php`, built from `ALLOWED_ZONES`/`ZONE_LABELS`, alongside the existing overall status panel
- [x] Updated `assets/js/dashboard.js` (`renderZones()`) to read `data.zones` from the API and flip each card's color/text live
- [x] Added a **Zone** column to the history table (both the `<thead>` in `index.php` and `renderTable()` in dashboard.js)
- [x] Total/today counters unchanged (still combined across all zones) — verified live via `get_motion_logs.php`
- [x] Added matching `.zone-card` / `.zone-dot` styles to `assets/css/style.css`, reusing the existing green/red color system
- [x] Verified live: dashboard HTML contains all 3 zone cards, and the API returns correct `zones` + `zone`/`zone_label` fields

---

## PHASE 8 — FULL SYSTEM TEST

- [x] Start Apache + MySQL
- [x] Upload the updated sketch, close Serial Monitor
- [x] Start the serial bridge
- [x] Open the dashboard
- [x] Trigger Room A only → confirm only Room A's indicator changes
- [x] Trigger Room B only → confirm only Room B's indicator changes
- [x] Trigger Room C only → confirm only Room C's indicator changes
- [x] Trigger two zones at once → confirm both update independently, no cross-talk
- [x] Confirm no duplicate/flooded rows from any zone
- [x] Confirm phpMyAdmin rows show the correct zone for each event

---

## PHASE 9 — DOCUMENTATION

- [ ] Update `README.md` to describe the 3-zone system
- [ ] Update `motion_sensor.ino`'s header comment with the new wiring (3 pins)
- [ ] Take a photo of the finished 3-sensor breadboard setup
- [ ] Take dashboard screenshots showing all 3 zones (idle + at least one active)
