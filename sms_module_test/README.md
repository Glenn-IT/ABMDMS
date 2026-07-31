# SMS Module Test — SIM800L V2.2 (UNV)

**Goal of this folder:** prove that the SIM800L module can send **one real 2G text
message** to your phone. Nothing else.

This is deliberately **separate from the ABMDMS system**. No PIR sensors, no XAMPP,
no MySQL, no dashboard, no `motion_sensor.ino`. Just the Arduino, the module, and
your phone. If a text does not arrive here, no amount of PHP or JavaScript will fix
it — so this test comes first.

When this folder's tests pass, go back to `SIM800L_SMS_CHECKLIST.md` Phase 4 and
carry on with the real system.

---

## 📍 STATUS — as of 31 July 2026

**The module works. A real SMS was sent and received on `+639169751409` (`+CMGS: 184`).**

| Test | Status |
|---|---|
| A — power + status LED | ✅ PASS — one blink every ~3 s |
| B — manual AT commands | ✅ PASS — text arrived on the phone |
| C — automatic self-test | ⬜ next |
| D — battery / laptop off | ⬜ blocked on the power-bank cutoff |

Full write-up of every fault hit along the way: **`RESULTS.md`**.

### ▶ START HERE NEXT TIME

1. **Set `SEND_REAL_SMS = true`** in `sim800l_selftest/sim800l_selftest.ino`
   *(your number is already in it — no other edit needed)*
2. Upload, Serial Monitor at 9600, wait for the boot self-test
3. Press `t` to run it again. **Run it three times.**
   Passing once is luck; passing three times is a working build.

### ⚠ Two things not yet fixed

- **The supply path is still soft.** The restart loop (Fault 5 in `RESULTS.md`) stopped,
  but nothing was changed to *make* it stop. Before the demo: run `5Vin` / `GND` as a
  short thick direct pair from the adapter to the module, bypassing the breadboard, with
  the capacitor on the module's own terminals.
- **Test D needs the power bank sorted** — "always on" mode, or the Arduino sharing the
  same bank, or a 47 Ω 1 W bleeder. See `POWER_TROUBLESHOOTING.md`, Symptom 2.

### Blanks worth filling in

The adapter's amp rating, the network, and the `AT+CSQ` / `AT+CSCA?` readings were never
written down. They're what tells you whether this build is reliable or merely lucky —
the table is waiting in `RESULTS.md`.

```
sms_module_test/
├── README.md                      <- you are here (the plan)
├── AT_COMMANDS.md                 <- what to type, what the answers mean
├── RESULTS.md                     <- fill this in as you go (your evidence)
├── POWER_TROUBLESHOOTING.md       <- faults hit on this build + how they were fixed
├── diagram.html                   <- test-rig wiring diagram (open in a browser)
├── breadboard.html                <- hole-by-hole breadboard layout + rail assignments
├── sim800l_passthrough/
│   └── sim800l_passthrough.ino    <- TEST B: you type AT commands by hand
└── sim800l_selftest/
    └── sim800l_selftest.ino       <- TEST C: runs the whole check automatically
```

---

## The 4 tests, in order

| # | Test | What it proves | Tool |
|---|---|---|---|
| A | Power + status LED | The module is alive and joined a 2G network | eyes + multimeter |
| B | Manual AT commands | The Arduino can talk to it, and it can send a text | `sim800l_passthrough.ino` |
| C | Automatic self-test | The same thing hands-free, with a PASS/FAIL verdict | `sim800l_selftest.ino` |
| D | Battery test | It still sends with the USB cable unplugged | power bank |

Do them in order. **Do not skip A.** Most "broken SIM800L" modules are a power or
coverage problem, and A catches both in two minutes.

---

## Before you start — the 60-second SIM check

Do this with a **normal phone**, not the module:

1. Put the SIM in a phone, in **the room you will demo in**.
2. Force the phone to **2G / GSM only** (Settings → Mobile network → Preferred network type).
3. Send a text from it. Did it arrive?
4. Turn the SIM's **PIN lock OFF**.
5. Confirm it has load / credit and is regular size (**2FF**).

SIM800L is **2G only**. Globe and Smart are switching 2G off area by area in the
Philippines. If step 3 fails, the module can never work in that room and no wiring
change will help.

- [ ] SIM sends a text on 2G, from the demo room
- [ ] PIN lock off
- [ ] Has load
- [ ] 2FF size

---

## TEST A — Power and the status LED

Wire **only** power for this test. No data pins yet. See `diagram.html`.

| Module pin | Goes to |
|---|---|
| `5Vin` | power bank / 5 V 2 A adapter **+** |
| `GND` | power bank **−** |
| 1000 µF cap | **across** `5Vin` ↔ `GND` (two legs, parallel — stripe leg on GND) |
| antenna | screwed on **before** power |
| `VDD` | leave unconnected — it is a 2.8 V *output*, not a power input |

Then:

- [ ] Antenna attached **before** power was applied
- [ ] Multimeter at the module's `5Vin`/`GND`: reads ~5 V and does not collapse
- [ ] Leave it running **2 full minutes** — the power bank must not switch itself off

**Now read the status LED. This is the whole test:**

| Blink pattern | Meaning | Do next |
|---|---|---|
| once every **~3 s** | ✅ joined the 2G network | go to Test B |
| once every **~1 s** | ❌ still searching — no coverage / no antenna / SIM problem | fix this first, Test B cannot pass |
| **off / nothing** | ❌ no power, or the cap is wired in series | recheck power and the capacitor |

> The single most common wiring mistake: drawing/building the 1000 µF capacitor
> **in line with** the 5 V wire instead of **across** the two rails. In series it
> blocks DC and the module never powers up at all.

---

## TEST B — Manual AT commands (the real proof)

Now add the three data wires:

| Module | Arduino |
|---|---|
| `TXD` | Pin **10** (direct) |
| `RXD` | Pin **11** (direct — V2.2 is 5 V-logic, **no** divider) |
| `RST` | Pin **12** (optional) |
| `GND` | Arduino `GND` **as well as** the supply `GND` — all three grounds joined |

1. Open `sim800l_passthrough/sim800l_passthrough.ino` in the Arduino IDE and upload it.
2. Open **Tools → Serial Monitor**, set **9600 baud**, and set the line ending to
   **"Both NL & CR"** (bottom-right dropdown — if this is wrong, nothing answers).
3. Type the commands from `AT_COMMANDS.md`, one at a time.

The finish line for this test:

- [ ] `AT` → `OK`
- [ ] `AT+CPIN?` → `+CPIN: READY`
- [ ] `AT+CSQ` → first number **above 10** (99 = no signal at all)
- [ ] `AT+CREG?` → `0,1` (home) or `0,5` (roaming)
- [ ] `AT+CMGF=1` → `OK`
- [ ] `AT+CMGS="+639XXXXXXXXX"` → `>` , type a message, press **Ctrl+Z** → `+CMGS: <n>`
- [ ] **A real text arrived on your phone** 🎉

Write the numbers you saw into `RESULTS.md`.

---

## TEST C — Automatic self-test

Same wiring as Test B. This one does the whole sequence for you and prints a
verdict, so you can repeat it quickly and show it to someone.

1. Open `sim800l_selftest/sim800l_selftest.ino`.
2. **Put your own number in `TEST_RECIPIENT`** at the top, international format:
   `+639171234567`.
3. Set `SEND_REAL_SMS` to `true` when you are ready to actually spend one text.
   Leave it `false` to check everything *except* the send.
4. Upload, open the Serial Monitor at **9600**.
5. Type `t` in the Serial Monitor to run the test again without re-uploading.

Expected output:

```
=== SIM800L SELF TEST ===
[1/6] Module responds (AT) ............ PASS
[2/6] SIM ready (AT+CPIN?) ............ PASS
[3/6] Signal quality (AT+CSQ) ......... PASS  (rssi=17 good)
[4/6] Network registered (AT+CREG?) ... PASS  (0,1 home)
[5/6] Text mode (AT+CMGF=1) ........... PASS
[6/6] Send SMS to +639171234567 ....... PASS  (+CMGS: 42)

RESULT: ALL PASS - the module can send 2G SMS.
```

- [ ] Self-test reports **ALL PASS**
- [ ] The text arrived on the phone

If any line says FAIL, the printed reason maps to the fix table in `AT_COMMANDS.md`.

---

## TEST D — Battery / demo test

The whole point of the SMS feature is that alerts go out with the laptop off.

1. Keep `sim800l_selftest.ino` loaded with `SEND_REAL_SMS = true`.
2. Change the trigger: unplug USB, and instead power the **Arduino from the power
   bank over USB**, with a **separate 5 V tap** from the same bank to the module's
   `5Vin`. The self-test runs once automatically at boot.
3. Wait for the text.

- [ ] Text arrives with the laptop switched off / USB unplugged
- [ ] The power bank stayed on for the whole test (no low-current cut-off)

If the bank cuts out: double-press its button for "always on" mode, or use a 5 V
2 A wall adapter for the demo — the most reliable option.

---

## Verdict

| | |
|---|---|
| **All four tests pass** | The module works. Go to `SIM800L_SMS_CHECKLIST.md` **Phase 4**, put your number in `SMS_RECIPIENT` in `motion_sensor.ino`, and upload the real sketch. |
| **Test A fails** | Power or coverage. Not a code problem. |
| **Test B fails at `AT`** | Wiring: TX/RX swapped, no common ground, or the module has no external power. |
| **Test B fails at `AT+CSQ` / `AT+CREG?`** | 2G coverage or antenna. Retest the SIM in a phone forced to 2G. |
| **Test B fails at `AT+CMGS`** | No load, wrong number format (must start `+63`), or the module browns out mid-send → capacitor / supply. |

Record everything in `RESULTS.md` — that file is your evidence for the defence.

---

## What this test does NOT cover

Deliberately out of scope here, because they are already built and tested elsewhere
in the system:

- `record_sms.php` / the `sms_logs` table → `SIM800L_SMS_CHECKLIST.md` Phases 5–6
- The serial bridge's `SMS_SENT:` parsing → Phase 7
- The dashboard's SMS Alerts panel → Phase 8

Those only *record* an SMS. The **Arduino** is what sends it, and this folder proves
the Arduino can.
