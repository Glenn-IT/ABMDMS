# ABMDMS — 4th PIR (Room D, Pin 5) Upgrade Checklist

Tracks the work to go from **3 PIR / 3 zones** to **4 PIR sensors / 4 zones**
(Room A = Pin 3, Room B = Pin 4, Room C = Pin 2, existing; **Room D = Pin 5, new**).
Work through the phases **in order** — each depends on the one before it.

Mirrors `MULTI_ZONE_CHECKLIST.md`, which added Room A + Room B — same pattern,
just adding Room D on top of the existing 3-zone system.

> Tip: in most Markdown editors you tick a box by changing `[ ]` to `[x]`.

---

## PHASE 1 — HARDWARE WIRING

- [x] Wire PIR #4 "Room D" (new) — VCC/GND/OUT → breadboard rails / Pin 5
- [x] Set the new PIR's yellow jumper to the **H** position (same as the other 3)
- [x] Turn the new PIR's time-delay screw fully anti-clockwise (same as the other 3)
- [x] Double-check breadboard `(+)`/`(-)` rails aren't crossed or shorted
- [x] Confirm the other 3 PIRs (Pins 2, 3, 4) are still wired correctly and undisturbed
- [x] Connect Arduino Uno to laptop, confirm green power LED is ON

---

## PHASE 2 — VERIFY THE NEW SENSOR ALONE (Serial Monitor, no code changes yet)

- [x] Open Arduino IDE Serial Monitor at 9600 baud
- [x] Temporarily test Room D on Pin 5 with a throwaway sketch (or reuse an existing single-pin sketch pointed at Pin 5)
- [x] Confirm Room D triggers cleanly and goes quiet at rest
- [x] Isolation-test Room D if it fires with nobody near it (OUT → GND jumper trick)
- [x] Confirm Rooms A, B, C still work unchanged (unaffected by the new wiring)

---

## PHASE 3 — UPDATE THE ARDUINO SKETCH

- [x] Open `arduino/motion_sensor/motion_sensor.ino`
- [x] Add a pin constant for Room D (5) alongside Room A (3), Room B (4), Room C (2)
- [x] Duplicate the state-change tracking (`motionActive`, `lowStartedAt`) for Room D — 4th independent set, not shared with the others
- [x] Print Room D's event tokens: `ROOMD_MOTION_DETECTED` / `ROOMD_MOTION_STOPPED`
- [x] Keep the warm-up countdown, but confirm it now covers all 4 sensors before "System Ready"
- [ ] Upload and wait for **"Done uploading"**
- [ ] Serial Monitor: confirm all 4 zones report independently and stay silent at rest
- [ ] Confirm triggering Room D does **not** falsely trigger Rooms A, B, or C (and vice versa)

---

## PHASE 4 — DATABASE CHANGES

- [x] Open `database/database.sql`
- [x] No schema change needed — the existing `zone VARCHAR(20)` column already fits `'ROOMD'`
- [x] Updated `database.sql`'s comment to mention Room D
- [ ] Confirm in phpMyAdmin that `motion_logs` accepts `zone = 'ROOMD'` without any ALTER TABLE

---

## PHASE 5 — UPDATE PHP CONFIG + APIs

- [x] `config.php`: add `'ROOMD'` to `ALLOWED_ZONES`
- [x] `config.php`: add `'ROOMD' => 'Room D'` to `ZONE_LABELS`
- [x] Confirmed `api/record_motion.php` needs no code changes (it already validates against `ALLOWED_ZONES`, which now includes ROOMD) — updated its doc comment
- [x] Confirmed `api/get_motion_logs.php` needs no code changes (it already loops zones dynamically from `ALLOWED_ZONES`/`ZONE_LABELS`)
- [ ] Test with `Invoke-RestMethod` POSTs: valid zone (ROOMD) inserts correctly, and the read API returns correct per-zone status including Room D — test row cleaned up afterward

---

## PHASE 6 — UPDATE THE SERIAL BRIDGE

- [x] Updated both `serial/serial_reader.ps1` (the active bridge) and `serial/serial_reader.php` (kept in sync)
- [x] Updated the regex from `^(ROOMA|ROOMB|ROOMC)_(MOTION_DETECTED|MOTION_STOPPED)$` to `^(ROOMA|ROOMB|ROOMC|ROOMD)_(MOTION_DETECTED|MOTION_STOPPED)$`
- [x] Confirmed the duplicate-guard (keyed on the full raw line) still works per-zone by construction — Room D repeating never suppresses another zone
- [x] Confirmed both bridges still POST `event_type`, `zone`, and `source` to the API unchanged
- [ ] Restart `start_reader.bat` with all 4 sensors wired — confirm it echoes all 4 zones' raw lines and shows `[OK]` for each

---

## PHASE 7 — UPDATE THE DASHBOARD

- [x] Confirmed the zone cards in `index.php` are built dynamically from `ALLOWED_ZONES`/`ZONE_LABELS` — Room D's card appears automatically, no hardcoding found
- [x] Confirmed `assets/js/dashboard.js` (`renderZones()`) needs no changes — it reads `data.zones` generically
- [x] Confirmed the **Zone** column in the history table (`<thead>` in `index.php` and `renderTable()` in dashboard.js) displays Room D correctly with no code changes needed
- [x] Total/today counters unchanged (still combined across all zones)
- [x] Confirmed `.zone-card` / `.zone-dot` styles in `assets/css/style.css` apply correctly to the 4th card (`.zones` grid uses `auto-fit`, no fixed column count) — no changes needed
- [ ] Verify live: dashboard HTML contains all 4 zone cards, and the API returns correct `zones` + `zone`/`zone_label` fields including Room D

---

## PHASE 8 — FULL SYSTEM TEST

- [ ] Start Apache + MySQL
- [ ] Upload the updated sketch, close Serial Monitor
- [ ] Start the serial bridge
- [ ] Open the dashboard
- [ ] Trigger Room A only → confirm only Room A's indicator changes
- [ ] Trigger Room B only → confirm only Room B's indicator changes
- [ ] Trigger Room C only → confirm only Room C's indicator changes
- [ ] Trigger Room D only → confirm only Room D's indicator changes
- [ ] Trigger two or more zones at once (including Room D) → confirm all update independently, no cross-talk
- [ ] Confirm no duplicate/flooded rows from any zone
- [ ] Confirm phpMyAdmin rows show the correct zone for each event, including ROOMD

---

## PHASE 9 — DOCUMENTATION

- [ ] Update `README.md` to describe the 4-zone system
- [ ] Update `motion_sensor.ino`'s header comment with the new wiring (4 pins)
- [ ] Update `arduino/PIR_MULTI_ZONE_WIRING.md` (or add a note) to include Room D on Pin 5
- [ ] Take a photo of the finished 4-sensor breadboard setup
- [ ] Take dashboard screenshots showing all 4 zones (idle + at least one active)
