# SIM800L EVB Wiring — SMS Alerts for ABMDMS (HW-131 & 12V Wall Adapter)

Adds a **SIM800L EVB GSM module** to the existing 4-PIR setup so the Arduino
**texts your phone** the moment motion starts — even if the laptop and XAMPP
are switched off. Wiring only. Do not upload the new sketch until the module
answers `AT` on its own (Step 5 below).

| Part | Arduino pin | Status |
|------|--------|-------------|--------|
| **Arduino 5V Power** | Arduino **5V Pin** | Connected to HW-131 **5V Rail (+)** (powers Arduino) |
| **Arduino GND** | Arduino **GND Pin** | Connected to Breadboard **GND Rail (-)** |
| **Arduino USB** | USB to PC/Laptop | Serial data link only (dashboard bridge) |
| Room C PIR | Digital Pin **2** | unchanged |
| Room A PIR | Digital Pin **3** | unchanged |
| Room B PIR | Digital Pin **4** | unchanged |
| ~~Room D PIR~~ | ~~Digital Pin **5**~~ | **retired** — see `arduino/PIR_MULTI_ZONE_WIRING.md` |
| **5V Piezo Buzzer** | Digital Pin **8** | Positive (+) to Pin 8, Negative (-) to Breadboard GND rail |
| SIM800L **TXD** | Digital Pin **10** | direct wire |
| SIM800L **RXD** | Digital Pin **11** | direct on the V2.2 board (divider only for the bare module) |
| SIM800L **RST** | Digital Pin **12** | optional |
| SIM800L **VCC / 5Vin** | Breadboard TOP `+` rail | **HW-131 5V output** (fed by 12V wall adapter) |
| SIM800L **GND** | Breadboard TOP `-` rail | common GND with HW-131 and Arduino GND |

---

## ✅ THIS BUILD — HW-131 Power Supply + SIM800L V2.2 by UNV

Power is provided by a **12 V DC Wall Adapter** plugged into an **HW-131 (MB-102) Breadboard Power Supply Module**, which slots directly into the breadboard rails:

1. **HW-131 Module Setup:**
   - Plug the HW-131 module into the left end of the breadboard (columns 1–4).
   - Set **Top Rail Jumper** to **`5V`** (powers SIM800L `5Vin` and `GND`).
   - Set **Bottom Rail Jumper** to **`5V`** (powers Arduino Uno `5V`, all PIR sensors `VCC`, and buzzer).
   - Plug the **12 V wall adapter** into the HW-131 DC barrel jack (5.5mm × 2.1mm center-positive).
   - Press the latching push-button switch **ON** (power LED lights up).

2. **Arduino Power & Connection:**
   - **Arduino 5V pin** connects to the **HW-131 5V Rail (+)** (the Arduino draws operating power from the HW-131).
   - **Arduino GND pin** connects to the **Breadboard GND Rail (-)**.
   - **USB Cable** connects from Arduino to PC/Laptop **strictly for serial data communication** (running the serial reader bridge to feed MySQL & the live dashboard).

3. **SIM800L V2.2 (UNV) Module:**
   - **No buck converter, no diodes** — feed `5Vin` directly from the HW-131 5 V top rail.
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
| `5Vin` | Breadboard **TOP `+` rail** (HW-131 5 V) |
| `GND` (either header) | Breadboard **TOP `-` rail** (Common GND) |
| `TXD` | Arduino **Pin 10** (direct) |
| `RXD` | Arduino **Pin 11** (direct — **no** divider) |
| `RST` | Arduino **Pin 12** (optional) |
| `VDD` | leave unconnected |
| 1000µF cap | across `5Vin` ↔ `GND` (still strictly required for 2A bursts) |

> Power everything (Arduino, SIM800L, PIRs, Buzzer) from the **HW-131 via the 12 V wall adapter**.
> The Arduino connects over **USB strictly for serial data communication** with the computer.

**Still applies to your board:** the common ground, the 1000 µF capacitor, the SIM / antenna /
2G checks.

### Checking your diagram

1. **The capacitor is in parallel, not in series.** It must have **two legs**: `+` on `5Vin` (top `+` rail), `−` on `GND` (top `-` rail), both at the module end.
   ```
        HW-131 5V (+) ●────────────────┬──────────► SIM800L 5Vin
                                       │
                                   ┌───┴───┐
                                   │ 1000µF│   + leg here
                                   │  cap  │   − leg (stripe) below
                                   └───┬───┘
        HW-131 GND (−) ●───────────────┴──────────► SIM800L GND  (and Arduino GND)

        the cap sits ACROSS the two rails — current does not flow "through" it
   ```

2. **Common ground** — module `GND`, HW-131 `−`, Arduino `GND` must all be joined.
3. **The three data wires** — `TXD`→Pin 10, `RXD`→Pin 11, `RST`→Pin 12.
4. **`VDD` shown as left unconnected**, so nobody later mistakes it for a power input.

---

## ⚠ Read this before you connect anything

Three things kill a SIM800L or make it look "broken" when it is fine:

1. **Powering it from the Arduino's 5V pin.** It will not work. The module pulls
   up to **2 A** in short bursts while transmitting; the Uno's regulator can give
   about 0.5 A. Always use the HW-131 powered by the 12V adapter.
2. **Missing the 1000 µF capacitor.** The linear AMS1117 regulator on the HW-131
   supplies up to ~800mA-1A; the 1000µF capacitor stores the charge required for
   the rapid 2 A GSM bursts.
3. **Forgetting the common ground.** Serial data needs a shared 0 V reference.
   No common GND = the module never answers, even though both parts are powered.

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

## ASCII wiring diagram  *(legacy — drawn for the BARE module)*

> ⚠️ **This diagram still shows the 1kΩ/2kΩ divider on Pin 11, which your board
> does not use.** It is kept for the bare 3.7–4.2 V module only. On the
> **SIM800L V2.2 (UNV)** that this project actually uses, `PIN 11` wires
> **straight to `RXD`** with no resistors at all — mentally delete the two
> resistor boxes below and read that line as a plain wire. Everything else in
## ASCII wiring diagram (HW-131 Power Supply & SIM800L V2.2)

```
                            ARDUINO UNO
                       ┌───────────────────┐
                       │  USB (from Laptop)│──────────► Power + Serial Bridge
                       │  5V  ○            │           (leave 5V UNCONNECTED!)
                       │  GND ●────────────┼──────────► Breadboard (-) rail (col 6)
                       │                   │
                       │  DIGITAL          │
                       │  PIN 2  ●─────────┼──────────► Room C PIR OUT
                       │  PIN 3  ●─────────┼──────────► Room A PIR OUT
                       │  PIN 4  ●─────────┼──────────► Room B PIR OUT
                       │  PIN 5  ○         │           (retired — Room D is out)
                       │                   │
                       │  PIN 10 ●◄────────┼──────────┐ (SIM TXD direct)
                       │  PIN 11 ●─────────┼──────┐   │ (SIM RXD direct - no divider)
                       │  PIN 12 ●─────────┼──┐   │   │ (SIM RST optional)
                       │  GND ●            │──┼───┼───┼──┐
                       └───────────────────┘  │   │   │  │
                                              │   │   │  │
                                             RST RXD TXD GND
                                              │   │   │  │
                                        ┌─────┴───┴───┴──┴───────────────┐
                                        │         SIM800L V2.2           │
                                        │   ┌──────────┐      ▲ antenna  │
                                        │   │ SIM card │      │          │
                                        │   └──────────┘   [status LED]  │
                                        │  5Vin ●       GND ●            │
                                        └───┬─────────────┬──────────────┘
                                            │             │
                             ┌──────────────┴─────────────┴─┐
                             │    + 1000µF capacitor  −     │ (across 5Vin / GND)
                             └──────────────┬─────────────┬─┘
                                            │             │
    BREADBOARD POWER RAILS                  │             │
    ┌───────────────────────────────────────┴─────────────┴────────────────┐
    │ TOP (+) RAIL  [5V]  ●─────────────────┘             │                │
    │ TOP (-) RAIL  [GND] ●───────────────────────────────┘                │
    ├──────────────────────────────────────────────────────────────────────┤
    │ BOTTOM (-) RAIL [GND] ●───────── (All 3 PIR GNDs & Arduino GND)      │
    │ BOTTOM (+) RAIL [5V]  ●───────── (All 3 PIR VCCs)                    │
    └───────▲──────────────────────────────────────────────────────────────┘
            │
    ┌───────┴──────────────────────────────┐
    │  HW-131 BREADBOARD POWER MODULE      │
    │  - Top jumper: set to 5V             │
    │  - Bottom jumper: set to 5V          │
    │  - Power switch: ON                  │
    │  [DC BARREL JACK: 12V 1A/2A Adapter] │
    └──────────────────────────────────────┘
```

**Reading it in words:**

- The **12V wall adapter** plugs into the **HW-131 module**, which slots into the breadboard.
- HW-131 jumpers set both **Top and Bottom rails to 5V**.
- HW-131 feeds the **SIM800L (Top Rail)** and all **3 PIR sensors (Bottom Rail)**.
- Arduino Uno connects to PC via **USB** for programming and dashboard serial relay.
- **Arduino 5V is left unconnected** to prevent backfeeding regulators.
- **Arduino GND** is connected to the breadboard `-` rail for a shared 0V reference.
- **1000 µF capacitor** sits directly across the SIM800L `5Vin` and `GND` pins on the breadboard.
- `TXD` → Arduino Pin 10, `RXD` → Arduino Pin 11 (direct), `RST` → Arduino Pin 12 (direct).


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
