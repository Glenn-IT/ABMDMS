# PIR + SMS Test Rig

**One PIR sensor + one SIM800L + its own dashboard.** A small, self-contained
version of ABMDMS that proves the whole chain works before you wire the
four-room system.

```
PIR -> Arduino -> SIM800L -> your phone
                -> USB -> serial_reader.ps1 -> PHP API -> MySQL -> dashboard
```

Nothing in this folder touches the main system. It has its own database
(`pir_sms_test`), its own API, and its own dashboard. Break it, wipe it, or
delete the whole folder — the real ABMDMS data is untouched.

| | Main ABMDMS | This rig |
|---|---|---|
| Database | `motion_monitoring` | `pir_sms_test` |
| Tables | `motion_logs`, `sms_logs` | `motion_events`, `sms_events` |
| Zones | 4 (Rooms A–D, pins 2–5) | 1 (`ROOM1`, pin 2) |
| Dashboard | `/ABMDMS/` | `/ABMDMS/pir_sms_test/` |

---

## What's in here

| Path | What it is |
|---|---|
| `wiring.html` | **Open this first.** Wiring diagram, hole-by-hole breadboard layout, checks, troubleshooting. |
| `arduino/pir_sms/pir_sms.ino` | The sketch. One PIR, one SIM800L, non-blocking SMS. |
| `database/pir_sms_test.sql` | Creates the database and its two tables. |
| `index.php` | The dashboard. |
| `api/record_motion.php` | Saves one motion event. |
| `api/record_sms.php` | Saves one SMS result. Never sends a text. |
| `api/get_data.php` | Feeds the dashboard everything in one request. |
| `serial/start_reader.bat` | Double-click to start the bridge. |
| `serial/serial_reader.ps1` | Reads the COM port, posts to the API. |
| `serial/list_ports.bat` | Shows which COM port the Arduino is on. |

---

## Setup, in order

**1. Wire it.** Open `wiring.html` in a browser and follow it. Do the
continuity checks in section 05 before applying power — they take two minutes
and save hardware.

**2. Create the database.** XAMPP Control Panel → start **Apache** and
**MySQL** → go to <http://localhost/phpmyadmin> → **SQL** tab → paste the whole
of `database/pir_sms_test.sql` → **Go**.

**3. Check the dashboard loads.** Open
<http://localhost/ABMDMS/pir_sms_test/>. You should see **NO DATA YET**, zeroed
tiles, and empty tables. If you get a yellow database warning, step 2 did not
finish.

**4. Set your phone number.** Open `arduino/pir_sms/pir_sms.ino` and edit:

```cpp
const char* SMS_RECIPIENT = "+639169751409";
```

International format, no spaces. Then edit `SMS_RECIPIENT_DISPLAY` in
`config.php` to match, so the dashboard shows the right number.

**5. Upload the sketch.** Arduino IDE → open `arduino/pir_sms/pir_sms.ino` →
select **Arduino Uno** and the right port → Upload. Open **Serial Monitor** at
**9600 baud**. You should see:

```
Starting SIM800L, please wait...
   Signal: +CSQ: 18,0
SIM_READY
   Alerts will be sent to: +639...
Warming up the PIR sensor, please stay still (30 seconds)...
30... 29... 28...
System Ready
Waiting for motion...
```

Stay out of the sensor's view during the countdown. If you see `SIM_FAIL:...`
instead of `SIM_READY`, see the troubleshooting table at the bottom of
`wiring.html`.

**6. Start the bridge.** **Close the Serial Monitor first** — only one program
can hold a COM port. Then double-click `serial/start_reader.bat`. If it cannot
open COM5, run `serial/list_ports.bat` to find the real port and edit
`$ComPort` at the top of `serial_reader.ps1`.

**7. Wave at the sensor.** Within a few seconds you should get all four:

- the reader window prints `[OK] ROOM1_MOTION_DETECTED -> saved as record #1`
- then `[SMS SENT] ROOM1 -> saved as alert #1`
- **a real text message on your phone**
- both rows on the dashboard

---

## Testing without hardware

You can prove the website half works before the Arduino arrives. In PowerShell:

```powershell
Invoke-RestMethod -Uri "http://localhost/ABMDMS/pir_sms_test/api/record_motion.php" `
  -Method Post -Body @{ event_type='MOTION_DETECTED'; zone='ROOM1'; source='TEST' }

Invoke-RestMethod -Uri "http://localhost/ABMDMS/pir_sms_test/api/record_sms.php" `
  -Method Post -Body @{ zone='ROOM1'; status='SENT'; detail='TEST' }
```

Both should answer `success: True` with an `id`, and both rows should appear on
the dashboard within 3 seconds.

To prove the validation is doing its job, send junk — it must be refused and
saved nowhere:

```powershell
Invoke-RestMethod -Uri "http://localhost/ABMDMS/pir_sms_test/api/record_motion.php" `
  -Method Post -Body @{ event_type='DROP TABLE'; zone='ROOM1' }
```

---

## The serial protocol

The Arduino prints one token per line at 9600 baud.

| Line | Meaning | Saved as |
|---|---|---|
| `ROOM1_MOTION_DETECTED` | Movement started | `motion_events` |
| `ROOM1_MOTION_STOPPED` | Quiet for 2 seconds | `motion_events` |
| `SMS_SENT:ROOM1` | The network accepted the text | `sms_events` · SENT |
| `SMS_FAIL:ROOM1:<REASON>` | Send failed | `sms_events` · FAILED |
| `SMS_SKIP:ROOM1:COOLDOWN` | Blocked on purpose, under 60 s since the last | `sms_events` · SKIPPED |
| `SIM_READY` / `SIM_FAIL:<REASON>` | Start-up result | shown on screen only |

Failure reasons: `NOPROMPT` (no `>` from the module), `SENDFAIL` (module said
`ERROR`), `TIMEOUT` (no `+CMGS` in 30 s), `NOMODULE` (failed start-up),
`ERROR` (rejected the recipient).

Anything else the sketch prints is shown in the reader window but deliberately
not saved.

---

## Things worth knowing

**The cooldown is not a bug.** After a text goes out, the next 60 seconds of
movement produce `SMS_SKIP:ROOM1:COOLDOWN` instead of another text. Without it,
one person walking around drains your load in a minute. Motion is still
recorded every time — only the texting is throttled. Change
`SMS_COOLDOWN_MS` in the sketch if you want a different window.

**PHP never sends an SMS.** The Arduino does it directly through the SIM800L
and then reports what happened. That is why the number lives in the sketch and
`SMS_RECIPIENT_DISPLAY` in `config.php` is only a label.

**Only motion _start_ texts.** `MOTION_STOPPED` is recorded but never texted.

**Clearing the data before a demo:**

```sql
USE pir_sms_test;
TRUNCATE TABLE motion_events;
TRUNCATE TABLE sms_events;
```

---

## Related

- `wiring.html` — wiring, breadboard, checks, troubleshooting
- `../sms_module_test/AT_COMMANDS.md` — AT command reference and `+CMS ERROR` codes
- `../sms_module_test/POWER_TROUBLESHOOTING.md` — brownouts, power-bank cutoff, capacitor faults
- `../arduino/SIM800L_WIRING.md` — the full wiring reference
- `../SIM800L_SMS_CHECKLIST.md` — the ten-phase plan for the main system
