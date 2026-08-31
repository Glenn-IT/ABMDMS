# Multi-Zone PIR Wiring — Breadboard (3 sensors)

The full ABMDMS sensor side: **3 PIR sensors, 3 zones**, all sharing one pair
of breadboard power rails. Wiring only — the sketch already reads all three.
Verify each sensor with the Arduino IDE Serial Monitor before touching the
PHP/DB/dashboard side.

| Zone | Sensor | Arduino pin | Notes |
|------|--------|-------------|-------|
| Room C | PIR #1 | Digital Pin **2** | the original sensor — this is why Pin 2 is Room C |
| Room A | PIR #2 | Digital Pin **3** | |
| Room B | PIR #3 | Digital Pin **4** | |
| ~~Room D~~ | ~~PIR #4~~ | ~~Digital Pin **5**~~ | **Retired** — see below |
| **Buzzer** | **5V Piezo Buzzer** | Digital Pin **8** | Positive (+) to Pin 8, Negative (-) to Breadboard GND rail |

**Pin 5 / Room D is retired.** During bring-up that pin would not respond to
**two different sensors**, so Room D was taken out of the system. Two sensors
failing on the same pin points at the pin, the OUT wire or that sensor's rail
tap — not the sensors. To chase it, move the suspect sensor to Pin 6 or 7 and
retest with `pir_sms_test/arduino/one_pin_test/one_pin_test.ino`; if it works
there, Pin 5 or its wiring is the fault. Restoring Room D afterwards is four
small edits, listed in `README.md`.

The pin/zone order is not alphabetical and that is deliberate: Pin 2 held the
first sensor ever built, that sensor was Room C, and every layer of the system
has agreed with that ever since. `ZONE_NAME[]` in
`arduino/motion_sensor/motion_sensor.ino` is written in pin order, so it reads
`{ "ROOMC", "ROOMA", "ROOMB" }`. Change the wire, not the code.

> There is also a fully illustrated version of this build — schematic,
> hole-by-hole breadboard drawing, pre-power checks and troubleshooting — at
> `wiring.html` in the project root (linked from the dashboard navbar).

---

## Why a breadboard & HW-131 Power Supply

The **HW-131 (MB-102) Breadboard Power Supply Module** is powered by a **12V DC Wall Adapter** and slots directly into the breadboard rails. It provides clean, regulated 5V power to both rails, powering the Arduino, all PIR sensors, the buzzer, and the SIM800L:

```
HW-131 5V (Bottom + Rail)  ────────► Powers Arduino 5V pin + all 3 PIR VCC pins
HW-131 GND (Bottom - Rail) ────────► Arduino GND pin + all 3 PIR GND pins + Buzzer (-)
Arduino Pin 2 / 3 / 4      ────────► Dedicated wire per sensor OUT pin (never shared)
Arduino Pin 8              ────────► Dedicated wire to 5V Piezo Buzzer (+) leg
Arduino USB                ────────► Serial data connection to PC/laptop only
```

The HW-131 supplies both the PIR sensors & Arduino (on the bottom rail at 5V) and the SIM800L module (on the top rail at 5V).

---

## ASCII wiring diagram

```
                              ARDUINO UNO
                       ┌───────────────────────┐
                       │  USB (from Laptop)    │──► Serial Data Link (dashboard bridge)
                       │  5V  ●────────────────┼───────────────┐
                       │  GND ●────────────────┼────────────┐  │
                       │                       │            │  │
                       │  DIGITAL              │            │  │
                       │  PIN 2 ●──────────────┼── OUT (C)  │  │
                       │  PIN 3 ●──────────────┼── OUT (A)  │  │
                       │  PIN 4 ●──────────────┼── OUT (B)  │  │
                       │  PIN 5 ○   (retired)  │            │  │
                       │  PIN 8 ●──────────────┼── Buzz (+) │  │
                       └───────────────────────┘            │  │
                                                            │  │
    BREADBOARD (BOTTOM POWER RAILS)                         │  │
    ┌──────────────────────────────────────────────────────┐│  │
    │ (+) RED  RAIL [5V]  ●──●────────●────────●─────●     ││◄─┘ Arduino 5V Power
    ├──────────────────────────────────────────────────────┤│
    │ (-) BLUE RAIL [GND] ●──●────────●────────●─────●     │◄─── Arduino GND
    └───────▲────────────────────────────────────────┼─────┘
            │                                        │
    ┌───────┴──────────────────────────────┐         │
    │  HW-131 BREADBOARD POWER MODULE      │         │
    │  - Bottom jumper: set to 5V          │         │
    │  - Top jumper: set to 5V (for SIM)   │         │
    │  - Power switch: ON                  │         │
    │  [DC JACK: 12V 1A/2A Wall Adapter]   │         │
    └──────────────────────────────────────┘         │
                       │        │        │           │
                    PIR #1   PIR #2   PIR #3       BUZZER
                    Room C   Room A   Room B      [ 5V PIEZO ]
                    Pin 2    Pin 3    Pin 4       (+) Pin 8
                  ┌───────┐┌───────┐┌───────┐      │     │
                  │ ╭───╮ ││ ╭───╮ ││ ╭───╮ │     ┌┴─────┴┐
                  │ │dom│ ││ │dom│ ││ │dom│ │     │ (+) (-│
                  │ ╰───╯ ││ ╰───╯ ││ ╰───╯ │     └───────┘
                  │ V O G ││ V O G ││ V O G │            │
                  └─┬─┬─┬─┘└─┬─┬─┬─┘└─┬─┬─┬─┘            │
                    │ │ └────┴─┴─┴─────┴─┴─┴─────────────┴──► (-) rail (GND)
                    │ └─────────────────────────────────────► its own Arduino pin
                    └───────────────────────────────────────► (+) rail (5V)
```

*(V = VCC, O = OUT, G = GND — but always confirm the actual pin order printed
under **each** sensor's dome; it varies between HC-SR501 batches, and three
sensors from the same bag do not have to agree with each other.)*

**Plain description of every wire, if the ASCII art is hard to follow:**

1. **HW-131 Module:** Plugged into breadboard ends, 12V adapter plugged into barrel jack, jumpers on 5V.
2. **Arduino 5V Power:** Arduino `5V` pin → breadboard `(+)` red rail (col 3) — Arduino draws power from HW-131.
3. **Arduino GND:** Arduino `GND` → breadboard `(-)` blue rail (col 6).
4. **Arduino USB:** Connected to PC/laptop for serial data communications only.
5. **PIR #1 (Room C):** VCC → `(+)` rail (col 18) · GND → `(-)` rail (col 21) · OUT → Arduino **Pin 2**
6. **PIR #2 (Room A):** VCC → `(+)` rail (col 31) · GND → `(-)` rail (col 34) · OUT → Arduino **Pin 3**
7. **PIR #3 (Room B):** VCC → `(+)` rail (col 43) · GND → `(-)` rail (col 46) · OUT → Arduino **Pin 4**
8. **5V Piezo Buzzer:** `(+)` leg (longer pin) → Arduino **Pin 8** · `(-)` leg (shorter pin) → breadboard `(-)` rail (col 15)

---

## Set each sensor before wiring it

Every HC-SR501 ships with its jumper and screws at random positions. Do all
three while you can still turn them over easily.

- **Yellow jumper → H** (repeat-trigger). On `L` the sensor ignores everything
  for its whole delay period after a trigger, so a person still in the room
  reads as gone.
- **Time-delay screw → fully anti-clockwise** (shortest hold). The sketch
  decides when motion has stopped; a sensor holding its output high for minutes
  only hides that.
- **Sensitivity screw → mid-range** to start. Turn it down later if one sensor
  keeps firing at nothing.

---

## Build notes

- **Warm-up:** every PIR needs ~30–60s to stabilize after power-on. All three
  warm up together, so stay away from **all of them** during the countdown —
  motion or body heat near any sensor can throw off its baseline.
- **Pin order varies:** read the `V / O / G` labels printed directly under each
  sensor's dome before wiring — don't assume it matches a diagram or another
  sensor from the same bag.
- **Keep rails tidy:** short jumper wires, one sensor's leads not draped over
  another's, avoids accidental shorts across the rails. Spread the three
  sensors' rail taps out along the board rather than crowding them into
  neighbouring columns — you will be re-seating these wires.
- **Watch for a split rail:** most full-size boards break each rail in the
  middle, near column 30. With the sensors spread along the board, an
  unbridged split leaves the far ones dead while the near ones work fine.
- **Floating OUT pin = fires randomly:** if a sensor triggers with no one near
  it, isolation-test by jumpering its OUT pin straight to GND — if it goes
  quiet, the code is fine and that sensor's OUT wire isn't seated properly.
- **One sensor at a time:** wire and verify each sensor alone in the Serial
  Monitor before adding the next. Three sensors wired all at once give you
  nine possible pin-to-room mix-ups and no way to tell which one you have.

---

## Verifying

Upload `arduino/motion_sensor/motion_sensor.ino`, open the Serial Monitor at
**9600**, and wait for `System Ready`. Then wave at one sensor at a time and
confirm you get the room you expect — not just *a* room:

```
ROOMC_MOTION_DETECTED     <- Pin 2
ROOMA_MOTION_DETECTED     <- Pin 3
ROOMB_MOTION_DETECTED     <- Pin 4
```

Each is followed by `ROOMx_MOTION_STOPPED` about 2 seconds after the movement
ends. If waving at one room lights up another, two OUT wires are swapped —
move the wire, not the code.

Once all three are confirmed, the rest of the system already supports them:
`config.php` lists all three zones, both serial bridges match all three, and the
dashboard builds its zone cards from `ALLOWED_ZONES`.
