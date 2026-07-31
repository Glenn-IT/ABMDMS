# Power Troubleshooting — SIM800L

Faults found on this build, in the order they appeared. Power is the cause of
almost every "broken SIM800L", so this is kept as its own file.

**Rule of thumb:** the module's problems are 90% power, 9% coverage, 1% code.

---

## Symptom 1 — module powers on, then off, then on again (repeating)

**Status on this build: SOLVED — the capacitor was in backwards.**

### What it looks like

The module lights up, dies, lights up again, over and over. Often together with
a capacitor that gets hot, bulges, leaks, or simply stops working.

### The cause

A **reversed electrolytic capacitor**. Backwards, it does not store charge — it
conducts, more and more as it degrades, until it is effectively a short across
the supply. To the power bank that looks like a fault, so its **over-current
protection trips, waits, and retries**. That retry loop is the on/off/on you see.
Meanwhile the capacitor cooks itself.

One fault, both symptoms.

### The isolation test (2 minutes, no parts)

1. **Remove the capacitor completely.**
2. Power the module with `5Vin` + `GND` only, antenna attached.
3. Watch the status LED.

| Result | Meaning |
|---|---|
| Powers up and stays up | ✅ the capacitor was the fault |
| Still cycling on/off/on | Something else is loading the rail — a wiring short, or `VDD` wrongly connected |

Without a capacitor the module may still brown out while transmitting, but it
will **power up and stay powered**. That is the difference you are looking for.

### Fitting the replacement

- **The stripe on the can is the NEGATIVE leg.** It goes to `GND`. Check twice.
- On an uncut capacitor the **longer leg is positive**. Once the legs are trimmed,
  go by the stripe only.
- **Voltage rating ≥ 10 V.** 16 V or 25 V is better. Do not use 6.3 V on a 5 V rail.
- A capacitor that vented, bulged, leaked or smelled sharp is scrap. They do not recover.
- Twist or solder it **directly onto the module's own `5Vin` / `GND` terminals**,
  not through breadboard holes.
- Before powering up: multimeter on continuity across the two supply wires. A brief
  beep that stops as the capacitor charges is normal. A beep that continues is a short.

---

## How to test a capacitor — the full procedure

Written up on 1 August 2026, after a failed capacitor cost two days of debugging on
the integrated rig. **Test the capacitor early.** It is a two-minute check that rules
out the single most common cause of every symptom in this file.

### Step 1 — look at it

This catches most dead capacitors in seconds. Any one of these means **scrap it**:

- bulging or domed top
- crust, residue or wetness around the legs
- a split in the X / K score mark on the top
- a sharp chemical smell
- it got hot in use

Capacitors that have vented, bulged or leaked do not recover.

### Step 2 — take it out of the circuit

Measuring it in place reads the whole rest of the board, not the capacitor. Then
**discharge it**: short the two legs with a screwdriver or wire for a second. At
1000 µF / 5 V it holds about 12 mJ — completely safe, no meaningful spark.

### Step 3a — if the meter has a capacitance mode (`µF`, `nF`, or `-|(-`)

| Reading | Verdict |
|---|---|
| 800–1200 µF | Healthy — ±20% is normal for electrolytics |
| 500–800 µF | Aged and weak — replace it |
| Under 500 µF, or 0 | **Dead** |
| Will not settle, jumps around | **Dead** |

### Step 3b — if it does not, use resistance mode

Set to **20 kΩ or 200 kΩ**, red probe on `+`, black on the stripe leg, and *watch the
number move*. At 1000 µF the change takes a few visible seconds, so this works well.

| Behaviour | Verdict |
|---|---|
| Starts low, **climbs steadily** toward `OL` / infinity | Healthy — you are watching it charge |
| Sits at a low value (a few hundred Ω) and stays there | **Shorted / leaky.** This is what causes on/off/on cycling |
| Jumps straight to `OL` with no climb at all | **Open circuit — dead** |

Discharge it again before re-testing, or swapping the probes.

### Step 4 — the isolation test (no meter needed)

Remove the capacitor completely and power the module with `5Vin` + `GND` + antenna only:

| Result | Meaning |
|---|---|
| Powers up and **stays up** at idle | The capacitor was the fault |
| Still cycles on/off/on | Something else loads the rail — a short, or `VDD` wrongly connected |

Without a capacitor the module may still brown out *while transmitting*, but it will
**power up and stay powered when idle**. That is the difference you are looking for.

---

## Symptom 2 — everything works, then the power bank switches itself off

**Status on this build: this is the current issue.**

### What it looks like

The module joins the network — status LED blinking **once every ~3 seconds** — and
then after somewhere between 10 and 60 seconds the power bank turns off on its own.
The timing is roughly the same every attempt.

### The cause

**Low-current auto-cutoff.** Nearly every power bank shuts its output down when the
load drops below about **50–100 mA**, on the assumption that whatever was charging
has finished. A SIM800L that has already registered idles at only **10–20 mA**, so the
bank decides nothing is connected.

Note *when* it happens: while the module was still searching for a network it was
transmitting hard and staying above the threshold. It only falls below once it has
successfully joined — so this symptom is, perversely, a sign that the module is working.

### Fixes, best first

**1. Use a 5 V 2 A wall adapter for the bench tests.**
No cutoff logic at all. Tests B and C happen at a desk, so the battery is not proving
anything yet. Removes the variable entirely.

**2. Look for the bank's low-current / "always on" mode.**
Usually a **double-press** of the button. Not all banks have one.

**3. Power the Arduino from the same bank.**
An Uno draws ~50 mA, plus the PIRs and the LED. Added to the module's 20 mA that is
often enough to stay above the threshold on its own. The module-only bench rig is the
lowest-draw configuration in the whole project — the problem may simply not exist in
the finished build. Try this before buying parts.

**4. Add a bleeder resistor.** The deliberate fix if 2 and 3 fail.

| | |
|---|---|
| Value | **47 Ω** |
| Rating | **1 W** (or two 100 Ω ½ W in parallel) |
| Where | straight across the module's `5Vin` and `GND` |
| Draws | ~106 mA — comfortably above any cutoff threshold |
| Dissipates | ~0.5 W — it will get warm, keep it clear of the wiring |

Cost is ~100 mA of continuous drain. On a 10,000 mAh bank that is still tens of hours,
far more than any demo needs.

### Why this matters for Phase 9

**Test D / the demo moment** — USB unplugged, Arduino on the battery, the text still
arrives — is the whole point of the SMS feature. It needs the battery to stay on.
Sort this out with option 2, 3 or 4 before the demo. For Tests B and C right now, use
the wall adapter and get the SMS actually sending first.

---

## Symptom 3 — module restarts on every transmit

**Status on this build: SEEN, AND SOLVED — the capacitor had failed.**
Confirmed on the `pir_sms_test` rig, 1 August 2026. See "How it was actually fixed" below.

### What it looks like

Everything passes until `AT+CMGS` and Ctrl+Z, then the module resets mid-message.
Or the self-test passes once and fails the next two runs.

On the integrated rig it looked like this: the first text sends fine, then the status
LED goes dark, blinks fast for about 10 seconds, settles to one blink every ~3 seconds
— and the moment motion triggers another send, the whole cycle repeats. The module is
rebooting and re-registering after every transmit attempt.

### The serial fingerprint

In the log it shows up as a **`TIMEOUT` immediately followed by an `ERROR`**:

```
SMS_FAIL:ROOM1:TIMEOUT     <- module died mid-send, never returned +CMGS
SMS_FAIL:ROOM1:ERROR       <- it rebooted, and a reboot forgets AT+CMGF=1,
                              so the very next AT+CMGS is rejected outright
```

That pair, repeating, is close to conclusive. The `ERROR` is not a second, separate
fault — it is the *consequence* of the reset in the line above it. Text mode does not
survive a reboot, and nothing you typed turned it off.

### The cause

The transmit burst pulls up to **2 A** in a fast step. Anything with resistance
between the supply and the module turns that current into a voltage drop:

- thin or long USB cable (0.3 Ω × 2 A = **0.6 V gone** before the module sees it)
- breadboard spring contacts, rated around **1 A each**
- dupont jumper wire
- missing or undersized bulk capacitor

### The fix

Run `5Vin` and `GND` as a **direct pair from the supply to the module** — thickest wire
available, as short as possible, twisted together, bypassing the breadboard completely.
Keep the breadboard for the ground bridge and the PIR sensors only. Capacitor soldered
or twisted right at the module's terminals.

This single change fixes more SIM800L faults than anything else.

### How to confirm it

Multimeter on DC volts at the **module's own pins**, not at the supply, while it sends:

| Reading | Meaning |
|---|---|
| Steady ~5 V | power is fine, look at antenna / SIM / coverage |
| Dips at each transmit | brownout confirmed — shorten and thicken the supply path |

`AT+CBC` also reports the module's own supply voltage in millivolts (the last number),
measured *inside* the chip. A cheap meter is often too slow to catch a 577 µs dip, so
`AT+CBC` is the better instrument here. Healthy is roughly 4000 and up.

### How it was actually fixed on this build

**The capacitor had failed.** Not the adapter, not the wiring, not the code.

Two days were spent on this because the supply looked beyond suspicion — a 5 V **2 A
wall adapter**, and a **1000 µF 16 V** capacitor of the correct value, correct voltage
rating and correct polarity, physically present on the board. Everything was right on
paper. The capacitor had simply stopped working, and a dead capacitor is invisible
until you test it.

With the capacitor replaced, the rig ran **10 minutes with no restart at all** —
motion detected, texts going out, module staying registered throughout.

**The lesson worth keeping:** a component being *present, correct and correctly fitted*
is not the same as it being *functional*. The capacitor was the fourth thing suspected
when it should have been the first, because it is the cheapest and fastest thing in the
whole chain to test. Do Step 3 of the capacitor procedure above **before** rewiring
anything.

---

## Quick reference

| Symptom | First suspect |
|---|---|
| On / off / on repeating | Capacitor reversed, dead, or a short |
| Dead / bulging / hot capacitor | Reversed polarity, or rated under 10 V |
| Works, then the bank cuts out | Low-current auto-cutoff — load is too small |
| Resets during `AT+CMGS` | **Test the capacitor first**, then the supply path |
| `TIMEOUT` then `ERROR`, repeating | The module is rebooting — a reboot forgets `AT+CMGF=1` |
| Restarts about 10 s after each send | Brownout on the transmit burst |
| `Signal:` prints garbage | Command echo is on — send `ATE0` at start-up |
| Nothing at all from `AT` | Not power — no common ground, or TX/RX swapped |
| LED blinks once per second forever | Not power — antenna, SIM, or no 2G coverage |

Two things this table is trying to teach:

**Test the capacitor before you rewire anything.** It is the cheapest and fastest check
in the whole chain, and on this build it was the answer twice — once reversed, once
simply failed. Both times it was suspected late.

**Not every fault is a power fault.** The last two rows cost nothing to check and chasing
power when the problem is a missing ground wire wastes a lot of an afternoon.
