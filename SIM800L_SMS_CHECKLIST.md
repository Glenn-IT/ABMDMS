# ABMDMS — SIM800L EVB SMS Alert Checklist

Tracks the work to add **SMS alerts** to the existing 4-zone motion system.
When a PIR fires, the **Arduino itself** texts your phone through the SIM800L —
so alerts still go out when the laptop and XAMPP are switched off.

Work through the phases **in order** — each depends on the one before it.
Mirrors `PIN5_ROOMD_CHECKLIST.md` in style.

Full wiring instructions: `arduino/SIM800L_WIRING.md`.

> Tip: in most Markdown editors you tick a box by changing `[ ]` to `[x]`.

---

## PHASE 1 — CHECK THE SIM AND THE COVERAGE (do this FIRST)

Do not buy parts or wire anything until this passes. SIM800L is **2G only**, and
2G is being switched off in many parts of the Philippines.

- [ ] Put the SIM in a normal phone, force it to 2G, and confirm it can send a text **from the room where the system will run**
- [ ] Confirm the SIM has load / credit
- [ ] Turn the SIM's **PIN lock OFF** (Settings → SIM lock)
- [ ] Confirm the SIM is regular size (2FF), the size the module's holder takes

---

## PHASE 2 — POWER (the number one cause of "broken" SIM800L modules)

> **This build: SIM800L V2.2 (UNV) — the 5 V board.** No buck, no diodes. The buck steps
> below are struck out; feed `5Vin` straight from the power bank. Keep the cap + grounds.

- [x] Board identified: **SIM800L V2.2 by UNV**, the 5 V version → 5 V direct, no buck
- [x] ~~Decide the supply: 5 V board → direct · 3.7–4.2 V board → LM2596 buck at 4.0 V~~ (5 V board — direct)
- [x] ~~If using a buck: set it to 4.0 V with a multimeter before connecting~~ (no buck)
- [ ] Wire only **`5Vin` / `GND`** for now, plus the **1000 µF capacitor** across them (stripe = negative)
- [ ] Measure the voltage at the module — ~5 V, and it does not collapse
- [ ] Leave it powered for 2 minutes — confirm the power bank does **not** switch itself off (low-current cut-off)
- [ ] Antenna connected **before** power was applied
- [ ] Confirm the module is powered from the **power bank's `5Vin`**, NOT the Arduino's 5V pin or DC jack

---

## PHASE 3 — VERIFY THE MODULE ALONE (no Arduino code yet)

- [ ] Status LED blinks **once every ~3 seconds** = joined the network (once per second = still searching → fix before continuing)
- [ ] Wire the data pins: `TXD` → Pin 10 (direct), `RXD` → Pin 11 (**direct — V2.2 is 5 V-logic, no divider**), `RST` → Pin 12
- [ ] Leave **`VDD` unconnected** — it is a 2.8 V reference output, not a power pin; do not feed 5 V into it
- [ ] Confirm the external supply GND, the module GND, and the Arduino GND are all joined
- [ ] Upload the throwaway passthrough sketch from `arduino/SIM800L_WIRING.md`
- [ ] Serial Monitor at 9600, line ending **"Both NL & CR"**
- [ ] `AT` → `OK`
- [ ] `AT+CPIN?` → `+CPIN: READY`
- [ ] `AT+CSQ` → signal above 10 (99 = no signal at all)
- [ ] `AT+CREG?` → `0,1` or `0,5`
- [ ] `AT+CMGF=1` → `OK`
- [ ] Full manual send with `AT+CMGS` + Ctrl+Z → **a real text arrives on your phone**

> Do not go past this line until a text actually arrives. Everything after this
> is software, and software cannot fix a hardware or coverage problem.

---

## PHASE 4 — THE ARDUINO SKETCH

- [x] `motion_sensor.ino`: added `SoftwareSerial` on pins 10 / 11 plus the SMS settings block
- [x] `motion_sensor.ino`: `queueSms()` puts a zone in line when its motion **starts** (never on stop)
- [x] `motion_sensor.ino`: per-zone cooldown (`SMS_COOLDOWN_MS`, 60 s) so one person walking around cannot drain your load
- [x] `motion_sensor.ino`: `smsTick()` sends the message in small steps with **no `delay()`**, so motion detection never pauses during a send
- [x] `motion_sensor.ino`: `simSetup()` checks the module during the PIR warm-up and prints `SIM_READY` or `SIM_FAIL:<reason>`
- [x] `motion_sensor.ino`: prints `SMS_SENT:` / `SMS_FAIL:` / `SMS_SKIP:` tokens for the laptop to log
- [ ] **Put your own phone number in `SMS_RECIPIENT`** near the top of the sketch (international format, e.g. `+639171234567`)
- [ ] Upload and wait for **"Done uploading"**
- [ ] Serial Monitor: `SIM_READY` appears before the warm-up countdown finishes
- [ ] Wave at Room A → `ROOMA_MOTION_DETECTED`, then `SMS_SENT:ROOMA`, and a text arrives
- [ ] Wave at Room A again straight away → `SMS_SKIP:ROOMA:COOLDOWN` (proves the cooldown works)
- [ ] Wave at Room B **while** Room A's message is still sending → Room B is still detected and logged (proves the send is non-blocking)
- [ ] All 4 zones still report independently, with no cross-talk

---

## PHASE 5 — DATABASE

- [x] `database/database.sql`: added the new `sms_logs` table (STEP 4)
- [x] `database/database.sql`: added the upgrade note + SMS commands to the useful-commands list
- [ ] Run the `CREATE TABLE sms_logs` block once in the phpMyAdmin **SQL** tab (no re-import needed, `motion_logs` is untouched)
- [ ] Confirm `sms_logs` now appears in phpMyAdmin under `motion_monitoring`

---

## PHASE 6 — PHP CONFIG + API

- [x] `config.php`: added `SMS_RECIPIENT_DISPLAY`, `ALLOWED_SMS_STATUSES`, `SMS_RECENT_LIMIT`
- [x] `api/record_sms.php`: new endpoint, built the same way as `record_motion.php` (POST only, whitelist validation, prepared statement)
- [x] `api/get_motion_logs.php`: returns `sms_today`, `sms_last`, `sms_recipient`, `sms_logs`, and `last_sms` per zone — all in the **same single response**, no extra endpoint
- [ ] Set `SMS_RECIPIENT_DISPLAY` in `config.php` to the same number you put in the sketch (this one is display only)
- [ ] Test with `Invoke-RestMethod`: valid `zone` + `status` inserts a row
- [ ] Test that a bogus `status` is rejected with HTTP 400
- [ ] Delete the test rows afterwards

---

## PHASE 7 — SERIAL BRIDGE

- [x] `serial/serial_reader.ps1` (the active bridge): added `$SmsApiUrl` and `$SmsPattern`
- [x] `serial/serial_reader.ps1`: SMS tokens POST to `record_sms.php`, mapping `SENT`→SENT, `FAIL`→FAILED, `SKIP`→SKIPPED
- [x] `serial/serial_reader.php`: kept in sync with the same behaviour
- [x] SMS tokens deliberately **skip** the duplicate-guard (that guard is for PIR chatter; the Arduino's cooldown already limits alerts)
- [ ] Close the Serial Monitor, run `start_reader.bat`
- [ ] Trigger motion → the console shows both `[OK]` (motion) and `[SMS SENT]` (alert) lines

---

## PHASE 8 — DASHBOARD

- [x] `index.php`: new **SMS Alerts** panel (alerts today, last alert, recent alerts table)
- [x] `assets/js/dashboard.js`: `renderSms()` fills it from the existing poll — no second request, no second timer
- [x] `assets/css/style.css`: `.sms-status-sent` / `-failed` / `-skipped` pill colours
- [ ] Open the dashboard → the SMS panel shows the alerts, updating on its own
- [ ] Check the browser Network tab: still only **one** request per refresh

---

## PHASE 9 — FULL SYSTEM TEST

- [ ] Start Apache + MySQL
- [ ] Serial bridge running, Serial Monitor closed
- [ ] Trigger Room A → dashboard zone card changes, history row appears, text arrives, SMS panel shows `SENT`
- [ ] Repeat for Rooms B, C and D
- [ ] Trigger two zones at once → both handled, no cross-talk, no missed events
- [ ] Turn the module off and trigger motion → `SMS_FAIL` shows on the dashboard (a failure is visible, not silent)
- [ ] **The demo moment:** unplug the USB cable, power the Arduino from the power bank, trigger a PIR → **the text still arrives** with no computer involved
- [ ] phpMyAdmin: `motion_logs` and `sms_logs` both show correct rows

---

## PHASE 10 — DOCUMENTATION

- [x] `arduino/SIM800L_WIRING.md` written (wiring, power warnings, ASCII diagram, AT-command bench test)
- [ ] Update `README.md` to mention the SMS alert feature
- [ ] Take a photo of the finished breadboard with the SIM800L in place
- [ ] Screenshot the dashboard's SMS Alerts panel with real alerts in it
- [ ] Screenshot the text message on your phone
