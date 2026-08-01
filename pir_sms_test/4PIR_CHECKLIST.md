# 4-PIR UPGRADE CHECKLIST — pir_sms_test

Taking the test rig from **1 sensor (`ROOM1`, Pin 2)** to **4 sensors
(`ROOMC`/`ROOMA`/`ROOMB`/`ROOMD` on Pins 2/3/4/5)**, matching the main system
exactly. Same 9-phase shape as `MULTI_ZONE_CHECKLIST.md` and
`PIN5_ROOMD_CHECKLIST.md`.

**Target mapping** — Pin 2 = ROOMC, Pin 3 = ROOMA, Pin 4 = ROOMB, Pin 5 = ROOMD.
Not alphabetical, on purpose: Pin 2 held the first sensor ever built and the
whole system has agreed with that since.

> **Wiring already done?** Work through **`BRINGUP_CHECKLIST.md`** instead — it
> covers Phases 1, 2 and 8 below as one ordered switch-on procedure, with the
> expected serial output and what to do at each failure. Come back here
> afterwards to tick the phases off.

> ## ⚠️ OUTCOME: the rig ships with THREE rooms, not four
>
> Bring-up finished with **ROOMC (Pin 2), ROOMA (Pin 3), ROOMB (Pin 4)** working
> and **Pin 5 / Room D removed** — the pin would not respond to two different
> sensors, so it is out of the system until the fault is found.
>
> Everything in this checklist below still describes the four-zone work, because
> that is the work that was done and it is all still valid; only the zone *count*
> changed at the end. The restore steps are in `README.md` and take four small
> edits.
>
> Because two sensors both failed on Pin 5, suspect the **pin, the OUT wire, or
> that sensor's rail tap** — not the sensors. See the retirement note in
> `BRINGUP_CHECKLIST.md` Step 2b for how to chase it.

---

## PHASE 1 — HARDWARE WIRING

- [ ] Set all four sensors before wiring: **yellow jumper on H**, **time-delay screw fully anti-clockwise**, sensitivity mid-range
- [ ] Read the `V / O / G` silkscreen under **each** dome separately — four sensors from one bag need not agree
- [ ] Arduino `5V` → bottom `(+)` rail, Arduino `GND` → bottom `(-)` rail
- [ ] All four PIR `VCC` → bottom `(+)` rail, spread out along the board
- [ ] All four PIR `GND` → bottom `(-)` rail
- [ ] Each PIR `OUT` → its **own** Arduino pin (2 / 3 / 4 / 5) — never shared, never through a rail
- [ ] SIM800L half unchanged from the 1-sensor build (TXD→10, RXD→11 direct, RST→12, external 5 V, 1000 µF in parallel, `VDD` unconnected)
- [ ] Bridge the two `(-)` rails; confirm the two `(+)` rails are **not** bridged
- [ ] If the board's rails split near column 30, bridge both halves of both bottom rails
- [ ] Continuity-test each OUT wire against every other — none may beep

Reference: `wiring.html` sections 02–06, or `../arduino/PIR_MULTI_ZONE_WIRING.md`.

---

## PHASE 2 — VERIFY EACH SENSOR ALONE

Add sensors **one at a time**. Four at once gives sixteen possible pin-to-room
mix-ups and no way to tell which you have.

- [ ] Wire sensor 1 only → upload → Serial Monitor at 9600 → confirm `ROOMC_MOTION_DETECTED` / `ROOMC_MOTION_STOPPED`
- [ ] Add sensor 2 → confirm `ROOMA_*` and that sensor 1 still behaves
- [ ] Add sensor 3 → confirm `ROOMB_*`
- [ ] Add sensor 4 → confirm `ROOMD_*`
- [ ] Confirm all four are quiet at rest after the 30 s warm-up
- [ ] Any sensor firing at nothing: jumper its Arduino pin straight to GND. Quiet = its OUT wire is not seated.

---

## PHASE 3 — ARDUINO SKETCH  *(`arduino/pir_sms/pir_sms.ino`)*

- [x] Replaced scalar `PIR_PIN` / `ZONE_NAME` / `ZONE_TEXT` with `NUM_ZONES = 4` and three index-parallel arrays in pin order
- [x] Per-zone state arrays `motionActive[]`, `lowStartedAt[]` — initializer lists extended to four values
- [x] `setup()` loops `pinMode` over all four pins and prints the pin→room list at start-up
- [x] `loop()` wraps the edge detection and the 2 s `STOP_CONFIRM_MS` stop-confirm in a per-zone loop
- [x] Built-in LED lights while **any** room is active (`anyActive` recomputed each pass)
- [x] `queueSms(int zone)` — cooldown is now **per room** via `lastSmsAt[]` / `smsEverSent[]`
- [x] `SMS_IDLE` picks the first pending room and remembers it in `smsZone`; `smsFinish()` credits the cooldown to that room
- [x] Global `SMS_MIN_GAP_MS` rest between any two sends kept as-is (module limitation, not a per-room one)
- [x] SIM800L code (`simSetup`, `simReset`, `simDrain`, `simSaw`, ESC-on-failure, reset-after-3-fails) reused unchanged
- [x] Header comment rewritten: 4 sensors, pin/zone table, sensor jumper settings
- [ ] **Verify in the Arduino IDE** — this confirms every initializer list really is length 4

---

## PHASE 4 — DATABASE  *(`database/pir_sms_test.sql`)*

- [x] `motion_events.zone` default changed `ROOM1` → `ROOMC`
- [x] Added `idx_zone` to both `motion_events` and `sms_events` — the dashboard now asks "newest row for room X" four times per refresh
- [x] Added a migration block for rigs already imported (`CREATE TABLE IF NOT EXISTS` will not fix an existing table)
- [ ] Run **Fix A** (migrate `ROOM1` → `ROOMC`, add indexes) **or Fix B** (drop both tables and re-import) in phpMyAdmin
- [ ] `SELECT zone, COUNT(*) FROM motion_events GROUP BY zone;` shows no `ROOM1` rows left

---

## PHASE 5 — PHP CONFIG + APIs

- [x] `config.php`: `ALLOWED_ZONES = ['ROOMA','ROOMB','ROOMC','ROOMD']`
- [x] `config.php`: `ZONE_LABELS` for all four
- [x] `config.php`: `MOTION_RECENT_LIMIT` 20 → 40 (four sensors fill 20 rows in under a minute)
- [x] `api/record_motion.php`: default zone `ROOM1` → `ROOMC`; no validation logic changed, it already checks `ALLOWED_ZONES`
- [x] `api/record_sms.php`: default zone `ROOM1` → `ROOMC`; no validation logic changed
- [x] `api/get_data.php`: added a `zones` block built by looping `ALLOWED_ZONES` — per-room status, last motion, last SMS
- [x] `api/get_data.php`: top-level `status` now means "**any** room active" — the old "newest row wins" logic would have let a stop in one room hide a person in another

---

## PHASE 6 — SERIAL BRIDGE  *(`serial/serial_reader.ps1`)*

- [x] `$ZonePattern` → `^(ROOMA|ROOMB|ROOMC|ROOMD)_(MOTION_DETECTED|MOTION_STOPPED)$`
- [x] `$SmsPattern` → `^SMS_(SENT|FAIL|SKIP):(ROOMA|ROOMB|ROOMC|ROOMD)(?::(.+))?$`
- [x] Duplicate guard changed from one "last line seen" to a per-line hashtable — with four rooms the lines interleave, and a single variable let a repeat slip through whenever another room printed in between
- [x] `start_reader.bat` / `list_ports.bat` need no changes

---

## PHASE 7 — DASHBOARD

- [x] `index.php`: four room cards rendered from `ALLOWED_ZONES` (`id="zone-rooma"` etc.) — nothing hardcoded to four
- [x] `index.php`: **Zone** column added to the SMS Alerts table, colspans updated
- [x] `index.php`: navbar subtitle and status-banner meta updated
- [x] `assets/dashboard.js`: `drawZones()` collects `.zone-card` elements and toggles `zone-motion` / `zone-none`
- [x] `assets/dashboard.js`: zone cell added to the SMS table renderer
- [x] `assets/style.css`: `.zones` / `.zone-card` / `.zone-dot` rules, `auto-fit` grid, `prefers-reduced-motion` respected
- [ ] Verify live: four cards on the page, each turning red only for its own room

---

## PHASE 8 — FULL SYSTEM TEST

- [ ] Start Apache + MySQL
- [ ] Upload the sketch, **close the Serial Monitor**
- [ ] Start `serial/start_reader.bat`
- [ ] Open <http://localhost/ABMDMS/pir_sms_test/>
- [ ] Trigger Room A only → only Room A's card changes
- [ ] Trigger Room B only → only Room B's card changes
- [ ] Trigger Room C only → only Room C's card changes
- [ ] Trigger Room D only → only Room D's card changes
- [ ] Trigger two rooms at once → both update, no cross-talk
- [ ] Confirm one text per room, naming the right room
- [ ] Trigger the same room twice inside 60 s → second logs `SKIPPED`
- [ ] Trigger a **different** room immediately after → it still sends (proves the cooldown is per room)
- [ ] Confirm no duplicate or flooded rows from any room
- [ ] phpMyAdmin: every row carries the correct `zone`

---

## PHASE 9 — DOCUMENTATION

- [x] `README.md` rewritten for 4 sensors: pin/zone table, migration warning, per-room cooldown notes, updated test commands
- [x] `wiring.html` rewritten for 4 sensors: schematic, hole-by-hole breadboard, sensor settings, bring-up order, 4-sensor troubleshooting rows
- [x] `wiring.html` published as a shareable page for use at the bench
- [x] `../arduino/PIR_MULTI_ZONE_WIRING.md` updated from 3 sensors to 4
- [x] `../arduino/motion_sensor/motion_sensor.ino` — stale `1k/2k divider` comment on Pin 11 removed
- [x] `../arduino/SIM800L_WIRING.md` — legacy divider diagram labelled as bare-module-only
- [ ] Photo of the finished 4-sensor breadboard
