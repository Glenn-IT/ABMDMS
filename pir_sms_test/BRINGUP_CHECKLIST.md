# BRING-UP CHECKLIST — wiring is done, now switch it on

Work **top to bottom and do not skip ahead.** Each step proves one thing. If you
power everything on at once and something is wrong, you will not know whether it
is a wire, the sketch, the bridge, or the database — and you will spend an
evening finding out.

Roughly 30 minutes if nothing is wrong.

Related: `4PIR_CHECKLIST.md` (the full upgrade plan) · `wiring.html` (diagrams,
section numbers referenced below) · `README.md`.

---

## STEP 0 — Five minutes with a multimeter, before any power

Wiring "done" and wiring "correct" are different things, and every fault below
is silent until it damages something or wastes an hour. **Nothing is powered
yet — no USB, no wall adapter.** Set the meter to continuity (the beep setting).

- [x] Probe the **two `+` rails** against each other → must **NOT** beep
      *(if it beeps you have joined the Arduino's 5 V to the module's supply — find it now)*
- [x] Probe the **two `−` rails** against each other → **MUST** beep
      *(that is the ground bridge; without it the module cannot talk at all)*
- [x] Probe each rail **end to end** → must beep
      *(silence in the middle = your board's rails split near column 30; bridge both halves)*
- [x] Probe the module's `5Vin` wire against the Arduino's `5V` pin → must **NOT** beep
- [x] Probe **each of the four OUT wires against every other** (6 pairs) → none may beep
      *(a beep means two rooms are shorted and will always report as one)*
- [x] Look at the Arduino header: **exactly one** wire in each of pins 2, 3, 4, 5 — and nothing in any other digital pin
- [x] Capacitor sits across **two different rails**, not two holes in the same one
- [x] Capacitor's **striped leg is in the `−` rail** *(backwards, it gets hot and vents)*
- [x] Module's **`VDD` has nothing plugged into it at all** *(it is a 2.8 V output — 5 V kills the board)*
- [x] **Antenna screwed on** *(transmitting without one damages the radio)*
- [x] All four sensors: **yellow jumper on H**, **time-delay screw fully anti-clockwise**

Anything failing here — fix it before going on. See `wiring.html` section 06.

---

## STEP 1 — Power the module alone, read its LED

Plug in the **external 5 V adapter only**. Leave the Arduino's USB unplugged.
Watch the SIM800L's status LED for a full minute:

| What you see | What it means | Do |
|---|---|---|
| Blink every **~3 s** | Joined the 2G network | Go to Step 2 |
| Blink every **~1 s** | Still searching | Wait 60 s. Still 1 s? → antenna, SIM, or no 2G coverage |
| Nothing, or on-off-on-off | Power problem | Check capacitor polarity, then the adapter. See `../sms_module_test/POWER_TROUBLESHOOTING.md` |

- [x] LED settles into the slow ~3 s blink

> If it restarts in a loop, **test the capacitor before anything else** — a dead
> one cost two days on this project already (Fault 7 in `../sms_module_test/RESULTS.md`).

---

## STEP 2 — Test ONE sensor at a time with `one_pin_test.ino`

**This is the step that catches swapped wires, and it is the whole reason for
doing hardware before software.** Leave every sensor plugged in — this
sketch simply ignores the three it is not testing.

Arduino IDE → open **`arduino/one_pin_test/one_pin_test.ino`** → **Arduino Uno**
+ the right port. Then **Tools → Serial Monitor at 9600 baud**.

- [ ] Set your phone number first if you have not: `SMS_RECIPIENT` near the top

The sketch ships set to Pin 2, so the first upload needs no edit. For each
sensor: change the one line near the top, upload, test, move on.

```cpp
const int TEST_PIN = 2;   // then 3, then 4
```

*(Pin 5 is still supported by this sketch on purpose — it is the tool for
retesting the faulty pin. See the note at the end of Step 2b.)*

**Expected output** (Pin 2 shown):

```
============================================
  ONE-PIN TEST
  Watching Pin 2  ->  ROOMC  (Room C)
============================================
Starting SIM800L, please wait...
   Signal: 18 - ok
SIM_READY
Warming up the sensor on Pin 2 ... (30 seconds)
30... 29... 28...

Quiet test: watching Pin 2 for 10 seconds. Do not move near it.
   Result: 0 changes, HIGH for 0% of the time.
   PASS - sensor is quiet and settled.

System Ready
```

Then wave at **that one sensor**:

```
ROOMC_MOTION_DETECTED        (Pin 2)
SMS_SENT:ROOMC
ROOMC_MOTION_STOPPED         (Pin 2)
```

Work through each sensor. Tick a row only when its room name is correct **and**
the quiet test passed:

| | Pin | Expect | Quiet test PASS | Correct room | SMS sent |
|---|---|---|---|---|---|
| Sensor 1 | **2** | `ROOMC` | [x] | [x] | [x] |
| Sensor 2 | **3** | `ROOMA` | [x] | [x] | [x] |
| Sensor 3 | **4** | `ROOMB` | [x] | [x] | [x] |
| ~~Sensor 4~~ | ~~**5**~~ | ~~`ROOMD`~~ | — | — | — | **FAILED — pin retired, see below** |

You only need one real text per sensor. The 60-second cooldown will print
`SMS_SKIP:...:COOLDOWN` for repeat waves — that is correct behaviour, not a fault.

### One wave usually gives you several DETECTED/STOPPED pairs

This is normal, not a fault:

```
ROOMC_MOTION_DETECTED        (Pin 2)
ROOMC_MOTION_STOPPED         (Pin 2)
ROOMC_MOTION_DETECTED        (Pin 2)
SMS_SENT:ROOMC
ROOMC_MOTION_STOPPED         (Pin 2)
```

The HC-SR501 holds its output HIGH for a couple of seconds, drops it, then sees
your hand again as you pull it back — or sees you, sitting right next to it.
Standing close to a sensor set to minimum delay will retrigger it several times.
What matters is that it **settles** once you move away and stay still.

`SMS_SENT` also appears **late**, several lines after the detection that caused
it — sending a text takes 5–10 seconds and the sketch keeps watching the sensor
the whole time rather than freezing. So the SMS lines interleave with motion
lines. That is the non-blocking sender working as intended.

Worry only if it keeps cycling when you are **out of the room** — that is the
`NOISY` case above, and it means a floating OUT wire.

**Every event line prints the pin number too.** If you forget to change
`TEST_PIN` before uploading, you will see `(Pin 2)` while waving at a different
sensor — which tells you immediately, instead of letting you believe you tested
something you did not.

### What the quiet test is telling you

| Result | Meaning | Do |
|---|---|---|
| `PASS` | Sensor is settled and wired properly | Go on |
| `STUCK HIGH` | Sees motion constantly | Something is moving near it (fan, curtain, sunlight), or its time-delay screw is turned up — turn it fully anti-clockwise |
| `NOISY` | Flickered with nobody near it | Almost always a floating input — do the GND test below |

### When a sensor misbehaves

**Wrong room name?** Two OUT wires are swapped. Move the wire, not the code.

**Never fires at all, or `NOISY`?** Unplug that OUT wire from the Arduino and
jumper that pin straight to `GND`. Goes quiet → the sketch is fine and the wire
is not seated. Still noisy → the problem is that pin.

**Every sensor behaves badly?** Ground is not reaching the Arduino — every input is
floating. Recheck the ground bridge and that the Arduino's `GND` wire is in the
`−` rail, not the `+`.

Do not go on until every wired sensor passes. Everything after this assumes the mapping is right.

> **What this step does NOT prove.** `one_pin_test.ino` only ever looks at one
> pin, so it cannot tell you what the *other three* were doing while you waved.
> HC-SR501s sitting side by side on one breadboard all point into the same
> room, and each sees a ~110° cone several metres deep — so waving at one of
> them is very likely waving at all of them. You will not find that out here. You
> find it out in Step 2b.

---

## STEP 2b — Prove the sensors are physically separate

Upload **`arduino/pin_monitor/pin_monitor.ino`**. It has no zones, no SMS and no
logic at all — it just prints the raw level of every sensor pin, so whatever you see
is the hardware telling you the truth.

```
  Pin2  Pin3  Pin4
  ROOMC ROOMA ROOMB
  ----- ----- -----
  .     .     .       12s  (all quiet)
  #     .     .       15s  ROOMC                <- what you want
  #     #     #       15s  ROOMC ROOMA ROOMB    <- the problem
```

- [x] Stand well back → every column reads `.` and stays there
- [x] Wave at the **Pin 2** sensor → **only** the Pin 2 column shows `#`
- [x] Wave at the **Pin 3** sensor → **only** the Pin 3 column shows `#`
- [x] Wave at the **Pin 4** sensor → **only** the Pin 4 column shows `#`
- [ ] ~~Wave at the **Pin 5** sensor~~ — **Pin 5 never responded to any sensor. Room D is retired; see the note after this step.**

### If several columns light up from one wave

Nothing is broken — the sensors are genuinely all seeing you. On a bench they
are centimetres apart and pointing the same way. In the real building they will
be in different rooms with walls between them, and the problem disappears
on its own.

To keep testing on the bench, do any one of these:

- **Point them apart** — face them in different directions, or lay them flat
  facing the ceiling and wave directly over one.
- **Box them in** — a cardboard tube, a mug, or a cupped hand over the three you
  are not testing. PIR sees infrared, so cardboard blocks it completely.
- **Spread them out** — a metre apart, facing away from each other.
- **Turn the sensitivity screw down** on each — shortens the range so you
  have to be close to trigger one.

### If a column never lights, ever

That pin is not receiving anything. Check the OUT wire at both ends, then try a
**known-good sensor** on that pin. If a second sensor also fails there, the
sensor is not the problem — suspect the pin itself, the OUT wire, or that
sensor's power tap on the rails.

### If one column exactly copies another

Those two OUT wires are shorted together, or one pin is floating and coupling to
its neighbour. Continuity-test that pair.

Do not go on until each sensor can be triggered on its own.

---

### 📌 Pin 5 / Room D is retired

Pin 5 did not respond to **two different sensors**, so Room D has been taken out
of the system. The rig now runs three rooms: **ROOMC (Pin 2), ROOMA (Pin 3),
ROOMB (Pin 4)**. Nothing downstream expects a fourth — the dashboard, the APIs
and the bridge all build themselves from the zone list.

Because two sensors both failed there, the sensor is very unlikely to be the
fault. When you want to chase it:

- [ ] Move the suspect sensor to a **different pin** (6 or 7) and test with
      `one_pin_test.ino`. Works there → the sensor is fine and Pin 5 or its wire
      is the problem.
- [ ] Check that sensor's **VCC and GND taps** actually reach the rails — a tap
      one row off the rail looks identical and does nothing.
- [ ] Try a **different jumper wire** for the OUT run.
- [ ] Retest Pin 5 directly: `one_pin_test.ino` with `TEST_PIN = 5`. That sketch
      still supports Pin 5 deliberately, and prints a reminder that it is retired.

To put Room D back once it works, see the restore steps in `README.md` — four
small edits, none of them structural.

---

## STEP 3 — Switch to the real multi-sensor sketch

Only now, with every sensor individually proven **and** physically isolated:

Arduino IDE → open **`arduino/pir_sms/pir_sms.ino`** → Upload → Serial Monitor
at 9600.

Expected:

```
PIR + SMS Test Rig - 3 sensors, 1 SIM800L
   Pin 2 -> ROOMC
   Pin 3 -> ROOMA
   Pin 4 -> ROOMB
Starting SIM800L, please wait...
   Signal: 18 - ok
SIM_READY
   Alerts will be sent to: +639...
Warming up 3 PIR sensors, please stay still (30 seconds)...
30... 29... 28...
System Ready
Waiting for motion...
```

- [ ] All three `Pin n -> ROOMx` lines print
- [ ] `SIM_READY` appears (not `SIM_FAIL:...`)
- [ ] Signal reads **10 or higher** *(under 10 = weak, sends will be unreliable)*
- [ ] `System Ready` appears after the countdown

**Stay away from every sensor during the countdown.** They warm up together
and body heat near any of them throws off its baseline.

If you get `SIM_FAIL:` instead, the reason is in the name — the troubleshooting
table at the bottom of `wiring.html` lists every one.

---

## STEP 3b — If a room triggers whenever ANOTHER room sends a text

Symptom: you wave at Room A, and Room C reports motion too — but only once the
four-sensor sketch is running. `pin_monitor.ino` showed clean isolation.

That difference is the whole clue. `pin_monitor.ino` never uses the SIM800L;
`pir_sms.ino` does. **The GSM transmit burst is false-triggering a sensor.** It
pulls about 2 A for a few milliseconds and radiates hard from the antenna, and
PIR sensors are very sensitive to both. Notice the false room appears at the
moment the text starts going out, several seconds *before* `SMS_SENT` comes back.

### Prove it in two minutes

In `pir_sms.ino`, near the top:

```cpp
const bool SIM_ENABLED = false;      // was true
```

Upload and wave at each room. **No texts will be sent** — the motion half still
works normally.

- [ ] With `SIM_ENABLED = false`, waving at Room A triggers **only** Room A
- [ ] Same for every other room

If the phantom triggers stop, the SIM800L is confirmed as the cause. **Set it
back to `true`** and apply the fixes below.

If they continue with the radio off, it is not the module — go back to Step 2b
and look for a floating OUT wire.

### The fixes, cheapest first

- **Move the antenna.** Get it as far from the sensors as its lead allows, and
  route it *away* from the board rather than across it. This is usually the whole
  fix on its own.
- **Move the offending sensor** away from the module, or turn it to face away.
- **Shorten and thicken the module's `5Vin` and `GND` leads.** The 2 A burst
  returning through a long thin ground wire shifts the ground the Arduino is
  measuring against. Short, direct, not across the breadboard.
- **Keep the OUT wires short**, and not running parallel to the antenna or the
  module's power leads.
- **A 0.1 µF capacitor across each PIR's VCC and GND**, right at the sensor, if
  you have any spare.

### The software filter

The sketch now requires a pin to stay HIGH for `START_CONFIRM_MS` (150 ms)
before it believes it. A real person holds a PIR output HIGH for seconds, so
this costs nothing real and throws away short interference pulses.

That filter also tells you **which kind** of interference you have:

| After the filter | Meaning |
|---|---|
| Phantom triggers gone | The burst was coupling into the OUT **wire**. Fixed. |
| Phantom triggers remain | The burst is triggering the **sensor itself**. No software can fix that — move the antenna or the sensor. |

Raising `START_CONFIRM_MS` above ~300 ms is not the answer; if 150 ms did not do
it, the sensor is genuinely tripping and the fix is physical.

---

## STEP 4 — Confirm they all still work together, and the cooldown is per room

- [ ] Wave at each sensor in turn → each prints its own `ROOMx_MOTION_DETECTED`
- [ ] Wave at the **same** sensor twice within a minute → second prints `SMS_SKIP:ROOMx:COOLDOWN`, no second text *(correct — it protects your load)*
- [ ] Wave at a **different** sensor immediately after → that one **does** send

That last pair is the per-room cooldown working. If the second room is also
skipped, tell me — the cooldown is meant to be counted per room, not for the rig.

---

## STEP 5 — Database

XAMPP Control Panel → start **Apache** and **MySQL**.

- [ ] Apache running
- [ ] MySQL running
- [x] ~~Import `database/pir_sms_test.sql`~~ — **already done.** Your database
      exists, its 60 old rows were migrated from `ROOM1` to `ROOMC`, and the
      zone indexes are added. Nothing to do here.

Sanity check at <http://localhost/phpmyadmin> → SQL tab:

```sql
USE pir_sms_test;
SELECT zone, COUNT(*) FROM motion_events GROUP BY zone;
```

- [ ] Result shows `ROOMC 60` and **no `ROOM1` rows**

---

## STEP 6 — Dashboard loads

Open <http://localhost/ABMDMS/pir_sms_test/>

- [ ] **Three room cards** appear: Room A, Room B, Room C *(no Room D — it is retired)*
- [ ] No yellow database warning strip
- [ ] The SMS Alerts table has a **Zone** column
- [ ] Page shows old Room C history from the previous testing

---

## STEP 7 — Start the serial bridge

- [ ] **Close the Arduino Serial Monitor first** — only one program can hold a COM port
- [ ] Double-click `serial/start_reader.bat`
- [ ] It says it opened the port and is listening

Cannot open COM5? Run `serial/list_ports.bat`, find the real port, and edit
`$ComPort` at the top of `serial/serial_reader.ps1`.

---

## STEP 8 — The whole chain, one room at a time

For **each** sensor, wave at it and confirm all five happen. Cover the others — on a bench they all see the same room:

| | Room C | Room A | Room B |
|---|---|---|---|---|
| Reader prints `[OK] ROOMx_MOTION_DETECTED -> saved as record #n` | [ ] | [ ] | [ ] | [ ] |
| Reader prints `[SMS SENT]` or `[SMS SKIPPED]` for that room | [ ] | [ ] | [ ] | [ ] |
| **Only that room's card** turns red on the dashboard | [ ] | [ ] | [ ] | [ ] |
| A new history row appears with the right zone | [ ] | [ ] | [ ] | [ ] |
| Text arrives naming the right room *(if not on cooldown)* | [ ] | [ ] | [ ] | [ ] |

Then the cross-checks:

- [ ] Trigger two rooms at once → both cards go red independently, no cross-talk
- [ ] Trigger one room, wait for its card to go green (~2 s after you stop), confirm the **other three cards did not change**
- [ ] Leave it running 10 minutes with occasional movement → no module restarts, no flood of duplicate rows
- [ ] phpMyAdmin: every new row carries the correct `zone`

---

## STEP 9 — Finish up

- [ ] Tick Phases 1, 2 and 8 in `4PIR_CHECKLIST.md`
- [ ] Take a photo of the finished 4-sensor breadboard *(last open item in Phase 9)*
- [ ] Wipe the test data before a demo, if you want a clean start:

```sql
USE pir_sms_test;
TRUNCATE TABLE motion_events;
TRUNCATE TABLE sms_events;
```

---

## When it goes wrong

The full symptom → cause → fix table is at the bottom of `wiring.html`
(section 09). The five most likely at this stage:

| Symptom | Cause | Fix |
|---|---|---|
| Waving at Room A lights Room B | Two OUT wires swapped | Move the wire, not the code |
| One room never reports | That OUT wire not seated, or a rail tap one row off | GND-jumper test from Step 3 |
| Rooms A and C work, B and D never do | Bottom rails split in the middle, only one half fed | Bridge both halves near column 30 |
| Every room fires constantly | Ground not reaching the Arduino | Check the ground bridge and the Arduino's `GND` wire |
| First text sends, then `TIMEOUT` then `ERROR` repeatedly | Brownout — module dying mid-send | Fix the power. No sketch change cures this. `../sms_module_test/POWER_TROUBLESHOOTING.md` |
