# SIM800L EVB Wiring — SMS Alerts for ABMDMS

Adds a **SIM800L EVB GSM module** to the existing 4-PIR setup so the Arduino
**texts your phone** the moment motion starts — even if the laptop and XAMPP
are switched off. Wiring only. Do not upload the new sketch until the module
answers `AT` on its own (Step 5 below).

| Part | Arduino pin | Status |
|------|-------------|--------|
| Room C PIR | Digital Pin **2** | unchanged |
| Room A PIR | Digital Pin **3** | unchanged |
| Room B PIR | Digital Pin **4** | unchanged |
| Room D PIR | Digital Pin **5** | unchanged |
| SIM800L **TXD** | Digital Pin **10** | new — direct wire |
| SIM800L **RXD** | Digital Pin **11** | new — direct on the V2.2 board (divider only for the bare module) |
| SIM800L **RST** | Digital Pin **12** | new — optional |
| SIM800L **VCC / 5Vin** | ✗ NOT the Arduino | new — **external supply only** |
| SIM800L **GND** | Arduino GND **and** supply GND | new — must be common |

---

## ✅ THIS BUILD — SIM800L V2.2 by UNV (the 5 V board)

The module in this project is confirmed as the **SIM800L V2.2 (UNV)** — the **5 V version**
with an onboard regulator and level shifting. That makes the wiring much simpler than the
generic guidance below. For **this** board:

- **No buck converter, no diodes** — feed `5Vin` straight from the power bank's 5 V.
- **No resistor divider** — the board is 5 V-logic, so `RXD` wires **directly** to Pin 11.
- **`VDD` is NOT a power pin** — it is a ~2.8 V logic-reference *output*. Leave it unconnected;
  do not feed 5 V into it.

Its two headers are labelled:

| Header | Pins |
|---|---|
| Power | `5Vin`, `GND` |
| UART / TTL | `VDD`, `RXD`, `TXD`, `GND`, `RST` |

Simplified wiring for this board:

| V2.2 pin | Goes to |
|---|---|
| `5Vin` | Power bank **5 V** (direct) |
| `GND` (either header) | Power bank GND **and** Arduino GND |
| `TXD` | Arduino **Pin 10** (direct) |
| `RXD` | Arduino **Pin 11** (direct — **no** divider) |
| `RST` | Arduino **Pin 12** (optional) |
| `VDD` | leave unconnected |
| 1000µF cap | across `5Vin` ↔ `GND` (still required) |

> Power the Arduino from the power bank over **USB**, and run a **separate 5 V tap** from the
> same bank to the module's `5Vin`. Never power the module through an Arduino pin or the DC jack —
> the Uno's regulator can't handle the module's 2 A transmit bursts, and a 5 V power bank is too
> low for the DC jack anyway (it wants 7–12 V).

**Still applies to your board:** the common ground, the 1000 µF capacitor, the SIM / antenna /
2G checks, and the power-bank auto-shutoff warning. Skip only the buck and divider sections.

### Checking your own diagram

If you draw this yourself (Canva, Fritzing, on paper), the drawing is only right when all
four of these are visible in it:

1. **The capacitor is in parallel, not in series.** It is the mistake that is easiest to
   draw and hardest to spot. It must have **two legs**: `+` on `5Vin`, `−` on `GND`, both
   at the module end. Drawn *inline on* the 5 V wire it blocks DC and the module never
   powers up at all.

   ```
        power bank (+) ●────────────────┬──────────► 5Vin
                                        │
                                    ┌───┴───┐
                                    │ 1000µF│   + leg here
                                    │  cap  │   − leg (stripe) below
                                    └───┬───┘
        power bank (−) ●────────────────┴──────────► GND  (and Arduino GND)

        the cap sits ACROSS the two rails — current does not flow "through" it
   ```

2. **Three grounds joined at one point** — module `GND`, power-bank `−`, Arduino `GND`.
   A breadboard `−` rail is a fine place to join them, as long as all three land on the
   **same** rail and the rail is labelled `−`.
3. **The three data wires** — `TXD`→Pin 10, `RXD`→Pin 11, `RST`→Pin 12. Power alone gets
   you a module that joins the network and does nothing; the SMS needs these.
4. **`VDD` shown as left unconnected**, so nobody later mistakes it for a power input.

Also label the Arduino box, and show the antenna and the SIM holder — those two are build
steps 1 and 2 below and a diagram that omits them invites skipping them.

---

## ⚠ Read this before you connect anything  *(general reference — bare module)*

Three things kill a SIM800L or make it look "broken" when it is fine:

1. **Powering it from the Arduino's 5V pin.** It will not work. The module pulls
   up to **2 A** in short bursts while transmitting; the Uno's regulator can give
   about 0.5 A. The module browns out and reboots mid-message.
2. **Feeding 5 V to a 3.7–4.2 V board.** Instant, permanent damage. Check the
   silkscreen — see below.
3. **Forgetting the common ground.** Serial data needs a shared 0 V reference.
   No common GND = the module never answers, even though both parts are powered.

### Which board do you have?

"SIM800L EVB" is sold in two electrically different versions. **Look at the
printing next to the VCC pin.**

| Board | Marking near VCC | Feed it |
|---|---|---|
| SIM800L core / EVB (small blue-purple board, SIM holder on the back) | `3.7V–4.2V` / `VCC 4V` | **NOT 5 V.** Use a buck converter set to **4.0 V**, or a 3.7 V Li-ion cell. |
| SIM800L V2.0 (larger red board, onboard regulator) | `5V` / `DC 5V` | A 5 V supply directly. |

If you use an **LM2596 buck converter**, set its output to 4.0 V with a
multimeter **before** you connect the module to it. (Two 1N4007 diodes in series
is the cheap trick — 5 V − 1.4 V ≈ 3.6 V — but it sags under the transmit burst
and gives random failures. Use the buck.)

### About powering it from a power bank (5 V 3 A)

- The **3 A rating is plenty** — the module only needs ~2 A in bursts.
- **But most power banks switch themselves off** when the draw falls below
  ~50–100 mA, and an idle SIM800L only draws ~20 mA. So the bank cuts out after
  about half a minute and the alerts silently stop. Fixes, best first:
  1. Use a power bank with an **"always on" / low-current mode** (often a
     double-press of the button).
  2. Use a **5 V 2 A wall adapter** for the demo — the most reliable option.
  3. Add a bleeder load (100 Ω resistor + LED) to keep the draw above the cut-off.
- **A 1000 µF capacitor across VCC/GND at the module is required**, not optional.
  It stores the energy for the transmit bursts. Mind the polarity — the stripe
  is the negative leg.

---

## The voltage divider on Pin 11  *(bare module only — NOT needed on your V2.2)*

**Skip this whole section for the V2.2 board** — it is 5 V-logic and `RXD` wires straight to
Pin 11. The divider below is only for the bare 3.7–4.2 V module, whose `RXD` is not 5 V tolerant.

On the bare module, Arduino Pin 11 sends 5 V, so it must be brought down to about 3 V with two
resistors:

```
   Arduino
   Pin 11 ●───────[ 1 kΩ ]───────┬───────────► SIM800L RXD
                                 │
                             [ 2 kΩ ]
                                 │
   Arduino GND ●─────────────────┴─────────── GND

           5 V x  2k / (1k + 2k)  =  3.3 V     <- safe for the module
```

The other direction (SIM800L `TXD` → Arduino Pin 10) is wired **straight through**.
The module sends 2.8 V and the Uno counts anything above 3.0 V as a "1", so this
is slightly marginal — it works on most boards. If you get garbage characters
back from the module, that is the reason, and the fix is a proper bidirectional
logic level converter.

---

## ASCII wiring diagram

```
                            ARDUINO UNO
                       ┌───────────────────┐
                       │                   │
                       │  5V  ●            │──────────► breadboard (+) rail  (PIRs ONLY)
                       │  GND ●            │──────────► breadboard (-) rail  ──────┐
                       │                   │                                        │
                       │  DIGITAL          │                                        │
                       │  PIN 2  ●─────────┼──► Room C PIR OUT                      │
                       │  PIN 3  ●─────────┼──► Room A PIR OUT                      │
                       │  PIN 4  ●─────────┼──► Room B PIR OUT                      │
                       │  PIN 5  ●─────────┼──► Room D PIR OUT                      │
                       │                   │                                        │
                       │  PIN 10 ●◄────────┼──────────────────────┐                 │
                       │  PIN 11 ●─────────┼───[1kΩ]───┬──────┐   │                 │
                       │  PIN 12 ●─────────┼───────┐   │      │   │                 │
                       │                   │       │ [2kΩ]    │   │                 │
                       │  GND ●            │───────┼───┴──────┼───┼─────────┐       │
                       └───────────────────┘       │          │   │         │       │
                                                    │          │   │         │       │
                                                   RST        RXD TXD       GND     GND
                                                    │          │   │         │       │
                                              ┌─────┴──────────┴───┴─────────┴────┐  │
                                              │         SIM800L EVB              │  │
                                              │   ┌──────────┐      ▲ antenna     │  │
                                              │   │ SIM card │      │             │  │
                                              │   └──────────┘   [status LED]     │  │
                                              │  VCC ●                            │  │
                                              └───┬───────────────────────────────┘  │
                                                  │                                   │
                                    ┌─────────────┴──────────────┐                    │
                                    │  + 1000µF capacitor  −     │                    │
                                    └─────────────┬──────────────┘                    │
                                                  │                                   │
                                 ┌────────────────┴─────────────────┐                 │
                                 │   EXTERNAL POWER                 │                 │
                                 │   power bank 5V  (or buck 4.0V)  │                 │
                                 │   (+) ─────────────► SIM800L VCC │                 │
                                 │   (−) ─────────────────────────────────────────────┘
                                 └──────────────────────────────────┘
                                    the (−) MUST also reach Arduino GND
```

**Reading it in words:**

- Arduino 5V / GND still feed the breadboard rails for the **4 PIR sensors only**.
- The SIM800L gets its power from the **external supply**, never from the Arduino.
- The external supply's minus, the SIM800L GND, and the Arduino GND are all
  **joined together** — this is the wire people forget.
- Pin 11 reaches RXD **through the resistor divider**; Pin 10 comes back from
  TXD directly.

---

## Build order (do not skip steps)

1. **SIM card first.** Regular size (2FF), PIN lock **turned off** (test it in a
   phone), with load/credit.
   **Philippines note:** SIM800L is **2G only**, and 2G is being switched off in
   many areas by Globe and Smart. Test the SIM in a 2G-forced phone *in the room
   you will demo in* before you build anything on top of it.
2. **Antenna on before power.** Transmitting without an antenna damages the radio.
3. **Power alone.** Wire only VCC / GND / capacitor. Measure with a multimeter:
   the correct voltage, and it must not collapse. Leave it running 2 minutes to
   check the power bank does not switch itself off.
4. **Check the status LED.** Blinking **once every ~3 seconds = joined the
   network** — good. **Once per second = still searching** — stop here and fix
   the SIM / antenna / coverage. Nothing further will work until this is right.
5. **Talk to it by hand.** Wire the data pins, then flash this throwaway
   passthrough sketch and use the Serial Monitor at 9600 with the line ending set
   to **"Both NL & CR"**:

   ```cpp
   #include <SoftwareSerial.h>
   SoftwareSerial sim(10, 11);
   void setup() { Serial.begin(9600); sim.begin(9600); }
   void loop()  {
     if (sim.available())    Serial.write(sim.read());
     if (Serial.available()) sim.write(Serial.read());
   }
   ```

   Type these one at a time:

   | You type | Good answer | Meaning |
   |---|---|---|
   | `AT` | `OK` | the module is alive |
   | `AT+CPIN?` | `+CPIN: READY` | SIM in, not PIN locked |
   | `AT+CSQ` | `+CSQ: 15,0` | signal strength — want above 10, **99 = no signal** |
   | `AT+CREG?` | `+CREG: 0,1` or `0,5` | joined the network |
   | `AT+CMGF=1` | `OK` | plain-text message mode |
   | `AT+CMGS="+639171234567"` | `>` | ready for the message |
   | type your text, then **Ctrl+Z** | `+CMGS: 12` | sent! |

   **You must receive a real text on your phone at this step** before uploading
   the ABMDMS sketch. If it fails here, it is a hardware / SIM / coverage problem,
   not a code problem.
6. **Only now** upload `arduino/motion_sensor/motion_sensor.ino` — remembering to
   put your own number in `SMS_RECIPIENT` near the top of the file.

---

## If it does not work

| What you see | Usual cause |
|---|---|
| `SIM_FAIL:NOREPLY` | No external power, no common ground, or TX/RX swapped |
| `SIM_FAIL:NOSIM` | SIM not seated, or the PIN lock is still on |
| `SIM_FAIL:NONETWORK` | No antenna, no load, or no 2G coverage in that area |
| Module reboots when sending | Power supply too weak, or the 1000 µF capacitor is missing |
| Garbage characters back | The 2.8 V TXD level — use a logic level converter |
| Random failed sends | SoftwareSerial dropping characters. An **Arduino Mega** (real second serial port, `Serial1`) removes this problem entirely |

## Reminder about what sends the SMS

The **Arduino** sends the text. PHP and the dashboard only *record* it. That is
why the alert still works with the USB cable unplugged — and why changing the
recipient number means editing `SMS_RECIPIENT` in the sketch and uploading again,
not editing `config.php`.
