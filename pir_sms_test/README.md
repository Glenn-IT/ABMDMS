# PIR + SMS Test Rig

**Three PIR sensors + one SIM800L + its own dashboard.** A self-contained copy
of ABMDMS, writing to its own database so you can break it freely.

```
3x PIR -> Arduino -> SIM800L -> your phone
                  -> USB -> serial_reader.ps1 -> PHP API -> MySQL -> dashboard
```

| Sensor | Arduino pin | Zone | Dashboard |
|---|---|---|---|
| PIR 1 | Pin **2** | `ROOMC` | Room C |
| PIR 2 | Pin **3** | `ROOMA` | Room A |
| PIR 3 | Pin **4** | `ROOMB` | Room B |

Pin 2 is Room C on purpose — it held the first sensor ever built, and the main
system has mapped it that way ever since. This rig copies it exactly.

> **Room D / Pin 5 is removed, not deleted.** Pin 5 would not respond to two
> different sensors during bring-up, so Room D is out of the system until that
> is fixed. Because two sensors both failed on it, suspect the **pin, the OUT
> wire, or its rail tap** — not the sensors.
>
> To retest it: upload `arduino/one_pin_test/one_pin_test.ino` with
> `TEST_PIN = 5`. That sketch still supports Pin 5 on purpose.
>
> To put Room D back once it works, four small edits:
> 1. `arduino/pir_sms/pir_sms.ino` — `NUM_ZONES = 4`, add `5` / `"ROOMD"` / `"Room D"` to the three lists
> 2. `config.php` — add `'ROOMD'` to `ALLOWED_ZONES` and `ZONE_LABELS`
> 3. `serial/serial_reader.ps1` — add `|ROOMD` to both regexes
> 4. `arduino/pin_monitor/pin_monitor.ino` — `NUM_PINS = 4`, add `5` / `"ROOMD"`
>
> Nothing else is hardcoded to a zone count.

Nothing in this folder touches the main system. It has its own database
(`pir_sms_test`), its own API, and its own dashboard. Break it, wipe it, or
delete the whole folder — the real ABMDMS data is untouched.

> **✅ 1 August 2026 — working end to end.** Motion triggers a real SMS, and both the
> event and the SMS result land in MySQL. Ran **10 minutes continuously with no module
> restarts**.
>
> Getting there cost two days to a **failed 1000 µF capacitor** — correct value, correct
> 16 V rating, correct polarity, and simply dead. If this rig starts restarting after
> every send, **test the capacitor first**: the procedure is in
> `../sms_module_test/POWER_TROUBLESHOOTING.md`, and the full story is Fault 7 in
> `../sms_module_test/RESULTS.md`. The raw serial log from the fault is in `Issues.md`.
>
> Known-good setup: 5 V 2 A wall adapter, 1000 µF 16 V across the module's `5Vin`/`GND`.

| | Main ABMDMS | This rig |
|---|---|---|
| Database | `motion_monitoring` | `pir_sms_test` |
| Tables | `motion_logs`, `sms_logs` | `motion_events`, `sms_events` |
| Zones | 4 (Rooms A–D, pins 2–5) | 3 (Rooms A–C, pins 2–4) — Room D pending |
| Dashboard | `/ABMDMS/` | `/ABMDMS/pir_sms_test/` |

The wiring and the serial protocol are the same as the main system. The rig is
one zone short only because Pin 5 is faulty on this board; everything else —
database name, table names, URL — is what always differed.

> **⚠️ Upgrading a rig you already imported?** The old 1-sensor version used
> zone `ROOM1`, which is no longer valid. Re-running the `.sql` file will *not*
> fix an existing database — `CREATE TABLE IF NOT EXISTS` skips tables that are
> already there. Run one of the two fixes at the bottom of
> `database/pir_sms_test.sql`: either migrate `ROOM1` → `ROOMC` and add the
> missing index, or drop both tables and re-import.

---

## What's in here

| Path | What it is |
|---|---|
| `wiring.html` | **Open this first.** Schematic, hole-by-hole breadboard layout, sensor settings, checks, troubleshooting. Covers the main system's wiring too. |
| `BRINGUP_CHECKLIST.md` | **Wiring finished? Start here.** Step-by-step switch-on, in the order that finds faults fastest. |
| `4PIR_CHECKLIST.md` | The phase-by-phase plan for the multi-sensor upgrade. |
| `arduino/pir_sms/pir_sms.ino` | The sketch. Three PIRs, one SIM800L, non-blocking SMS, per-room cooldown. |
| `arduino/one_pin_test/one_pin_test.ino` | Bring-up sketch: watches **one** sensor, with SMS. Change `TEST_PIN` (2/3/4, or 5 to retest the faulty pin) and re-upload. |
| `arduino/pin_monitor/pin_monitor.ino` | Diagnostic: prints the raw level of every sensor pin, no logic at all. Use it when several rooms trigger at once, or to test a suspect pin. |
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

**1. Wire it.** Open `wiring.html` in a browser and follow it. Set each
sensor's jumper and screws (section 05) before wiring, and do the continuity
checks (section 06) before applying power — they take two minutes and save
hardware. **Add the sensors one at a time**, confirming each in the Serial
Monitor before wiring the next.

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
PIR + SMS Test Rig - 3 sensors, 1 SIM800L
   Pin 2 -> ROOMC
   Pin 3 -> ROOMA
   Pin 4 -> ROOMB
Starting SIM800L, please wait...
   Signal: +CSQ: 18,0
SIM_READY
   Alerts will be sent to: +639...
Warming up 3 PIR sensors, please stay still (30 seconds)...
30... 29... 28...
System Ready
Waiting for motion...
```

That pin-to-room list is your reference for checking the wiring: wave at one
sensor at a time and confirm you get the room you expect. Stay out of **every**
sensor's view during the countdown — they all warm up together. If you see
`SIM_FAIL:...` instead of `SIM_READY`, see the troubleshooting table at the
bottom of `wiring.html`.

**6. Start the bridge.** **Close the Serial Monitor first** — only one program
can hold a COM port. Then double-click `serial/start_reader.bat`. If it cannot
open COM5, run `serial/list_ports.bat` to find the real port and edit
`$ComPort` at the top of `serial_reader.ps1`.

**7. Wave at each sensor in turn.** Within a few seconds of each wave you should
get all five. Cover the sensors you are not testing — on a bench they all point
into the same room and will all trigger together:

- the reader window prints `[OK] ROOMC_MOTION_DETECTED -> saved as record #1`
- then `[SMS SENT] ROOMC -> saved as alert #1`
- **a real text message on your phone**, naming that room
- the matching **room card turns red** on the dashboard
- a new row in the history table with the right zone

Then check the cooldown is per-room: wave at the same sensor twice inside a
minute (the second one logs `SKIPPED`), then wave at a *different* sensor
immediately — that one must still send.

---

## Testing without hardware

You can prove the website half works before the Arduino arrives. In PowerShell:

```powershell
foreach ($z in 'ROOMA','ROOMB','ROOMC') {
  Invoke-RestMethod -Uri "http://localhost/ABMDMS/pir_sms_test/api/record_motion.php" `
    -Method Post -Body @{ event_type='MOTION_DETECTED'; zone=$z; source='TEST' }
}

Invoke-RestMethod -Uri "http://localhost/ABMDMS/pir_sms_test/api/record_sms.php" `
  -Method Post -Body @{ zone='ROOMC'; status='SENT'; detail='TEST' }
```

Each should answer `success: True` with an `id`, all three room cards should
turn red within 3 seconds, and the rows should appear in the history table.

To prove the validation is doing its job, send junk — it must be refused and
saved nowhere:

```powershell
Invoke-RestMethod -Uri "http://localhost/ABMDMS/pir_sms_test/api/record_motion.php" `
  -Method Post -Body @{ event_type='DROP TABLE'; zone='ROOMA' }

# ROOM1 is no longer a valid zone - this must also be rejected
Invoke-RestMethod -Uri "http://localhost/ABMDMS/pir_sms_test/api/record_motion.php" `
  -Method Post -Body @{ event_type='MOTION_DETECTED'; zone='ROOM1' }
```

---

## The serial protocol

The Arduino prints one token per line at 9600 baud. `ROOMx` below is any of
`ROOMA`, `ROOMB`, `ROOMC` — the room name is the only thing telling
the dashboard which sensor spoke.

| Line | Meaning | Saved as |
|---|---|---|
| `ROOMx_MOTION_DETECTED` | Movement started in that room | `motion_events` |
| `ROOMx_MOTION_STOPPED` | That room quiet for 2 seconds | `motion_events` |
| `SMS_SENT:ROOMx` | The network accepted the text | `sms_events` · SENT |
| `SMS_FAIL:ROOMx:<REASON>` | Send failed | `sms_events` · FAILED |
| `SMS_SKIP:ROOMx:COOLDOWN` | Blocked on purpose, under 60 s since that room's last | `sms_events` · SKIPPED |
| `SIM_READY` / `SIM_FAIL:<REASON>` | Start-up result | shown on screen only |

Failure reasons: `NOPROMPT` (no `>` from the module), `SENDFAIL` (module said
`ERROR`), `TIMEOUT` (no `+CMGS` in 30 s), `NOMODULE` (failed start-up),
`ERROR` (rejected the recipient).

Anything else the sketch prints is shown in the reader window but deliberately
not saved.

---

## Things worth knowing

**The cooldown is not a bug, and it is per room.** After a text goes out about
Room A, the next 60 seconds of movement *in Room A* produce
`SMS_SKIP:ROOMA:COOLDOWN` instead of another text. Without it, one person
pacing around drains your load in a minute. But the other three rooms are
unaffected — somebody walking from Room A into Room B texts you again
immediately, which is exactly the event you care about. Motion is still
recorded every time; only the texting is throttled. Change `SMS_COOLDOWN_MS`
in the sketch if you want a different window.

**One text at a time.** The SIM800L can only send one message at a time, so if
two rooms trigger together the second alert waits a few seconds and then goes
out. It is queued, not dropped. The sketch never blocks while waiting, so no
motion is missed during a send.

**Every room, one LED.** The Arduino's built-in LED on pin 13 lights when
**any** room is active. It cannot tell you which one — the Serial Monitor and
the dashboard cards do that.

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
- `4PIR_CHECKLIST.md` — the phase plan for the 1 → 4 sensor upgrade
- `../arduino/PIR_MULTI_ZONE_WIRING.md` — the same sensor wiring, for the main system
- `../sms_module_test/AT_COMMANDS.md` — AT command reference and `+CMS ERROR` codes
- `../sms_module_test/POWER_TROUBLESHOOTING.md` — brownouts, power-bank cutoff, capacitor faults
- `../arduino/SIM800L_WIRING.md` — the full wiring reference
- `../SIM800L_SMS_CHECKLIST.md` — the ten-phase plan for the main system
