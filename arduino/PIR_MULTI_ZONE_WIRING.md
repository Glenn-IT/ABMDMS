# Multi-Zone PIR Wiring — Breadboard (4 sensors)

The full ABMDMS sensor side: **4 PIR sensors, 4 zones**, all sharing one pair
of breadboard power rails. Wiring only — the sketch already reads all four.
Verify each sensor with the Arduino IDE Serial Monitor before touching the
PHP/DB/dashboard side.

| Zone | Sensor | Arduino pin | Notes |
|------|--------|-------------|-------|
| Room C | PIR #1 | Digital Pin **2** | the original sensor — this is why Pin 2 is Room C |
| Room A | PIR #2 | Digital Pin **3** | |
| Room B | PIR #3 | Digital Pin **4** | |
| Room D | PIR #4 | Digital Pin **5** | |

The pin/zone order is not alphabetical and that is deliberate: Pin 2 held the
first sensor ever built, that sensor was Room C, and every layer of the system
has agreed with that ever since. `ZONE_NAME[]` in
`arduino/motion_sensor/motion_sensor.ino` is written in pin order, so it reads
`{ "ROOMC", "ROOMA", "ROOMB", "ROOMD" }`. Change the wire, not the code.

> There is also a fully illustrated version of this build — schematic,
> hole-by-hole breadboard drawing, pre-power checks and troubleshooting — at
> `pir_sms_test/wiring.html`. It is written for the test rig, but the sensor
> wiring is identical.

---

## Why a breadboard

An Arduino Uno has only **one 5V pin** and a couple of GND pins — not enough
to plug 4 sensors in directly. The breadboard's two power rails (`+` and `-`
running the length of the board) let all 4 PIR sensors share a single 5V/GND
feed from the Arduino, while each sensor's **OUT** wire still runs to its own
dedicated Arduino digital pin.

```
Arduino 5V  ────────► breadboard  +  rail (red)   ─── shared by all 4 sensors
Arduino GND ────────► breadboard  -  rail (blue)  ─── shared by all 4 sensors
Arduino Pin 2/3/4/5 ► one dedicated wire per sensor OUT pin (never shared)
```

All four sensors together draw roughly 60 mA, which the Arduino's 5V pin
supplies comfortably. (The SIM800L is the opposite case — it needs its own
external supply. See `arduino/SIM800L_WIRING.md`.)

---

## ASCII wiring diagram

```
                              ARDUINO UNO
                       ┌───────────────────────┐
                       │  5V  ●                │───────────────────┐
                       │  GND ●                │────────────────┐  │
                       │                       │                │  │
                       │  DIGITAL              │                │  │
                       │  PIN 2 ●──────────────┼── OUT, Room C  │  │
                       │  PIN 3 ●──────────────┼── OUT, Room A  │  │
                       │  PIN 4 ●──────────────┼── OUT, Room B  │  │
                       │  PIN 5 ●──────────────┼── OUT, Room D  │  │
                       └───────────────────────┘                │  │
                                                                │  │
    BREADBOARD                                                  │  │
    ┌──────────────────────────────────────────────────────┐    │  │
    │ (+) RED  RAIL ●──●────────●────────●────────●        │◄───┼──┘  Arduino 5V
    ├──────────────────────────────────────────────────────┤    │
    │ (-) BLUE RAIL ●──●────────●────────●────────●        │◄───┘     Arduino GND
    └──────────────────────────────────────────────────────┘
                       │        │        │        │
                    PIR #1   PIR #2   PIR #3   PIR #4
                    Room C   Room A   Room B   Room D
                    Pin 2    Pin 3    Pin 4    Pin 5
                  ┌───────┐┌───────┐┌───────┐┌───────┐
                  │ ╭───╮ ││ ╭───╮ ││ ╭───╮ ││ ╭───╮ │
                  │ │dom│ ││ │dom│ ││ │dom│ ││ │dom│ │
                  │ ╰───╯ ││ ╰───╯ ││ ╰───╯ ││ ╰───╯ │
                  │ V O G ││ V O G ││ V O G ││ V O G │
                  └─┬─┬─┬─┘└─┬─┬─┬─┘└─┬─┬─┬─┘└─┬─┬─┬─┘
                    │ │ └────┴─┴─┴─────┴─┴─┴────┴─┴─┴──► (-) rail   (all four)
                    │ └───────────────────────────────► its own Arduino pin
                    └─────────────────────────────────► (+) rail   (all four)
```

*(V = VCC, O = OUT, G = GND — but always confirm the actual pin order printed
under **each** sensor's dome; it varies between HC-SR501 batches, and four
sensors from the same bag do not have to agree with each other.)*

**Plain description of every wire, if the ASCII art is hard to follow:**

1. Arduino `5V` → breadboard `(+)` red rail
2. Arduino `GND` → breadboard `(-)` blue rail
3. **PIR #1 (Room C):** VCC → `(+)` rail · GND → `(-)` rail · OUT → Arduino **Pin 2**
4. **PIR #2 (Room A):** VCC → `(+)` rail · GND → `(-)` rail · OUT → Arduino **Pin 3**
5. **PIR #3 (Room B):** VCC → `(+)` rail · GND → `(-)` rail · OUT → Arduino **Pin 4**
6. **PIR #4 (Room D):** VCC → `(+)` rail · GND → `(-)` rail · OUT → Arduino **Pin 5**

That is 14 wires: 2 for the rails, and 3 per sensor. No resistors anywhere on
the PIR side — the HC-SR501 drives a clean digital level on its own, so there
are no pull-ups or pull-downs to add.

---

## Set each sensor before wiring it

Every HC-SR501 ships with its jumper and screws at random positions. Do all
four while you can still turn them over easily.

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

- **Warm-up:** every PIR needs ~30–60s to stabilize after power-on. All four
  warm up together, so stay away from **all of them** during the countdown —
  motion or body heat near any sensor can throw off its baseline.
- **Pin order varies:** read the `V / O / G` labels printed directly under each
  sensor's dome before wiring — don't assume it matches a diagram or another
  sensor from the same bag.
- **Keep rails tidy:** short jumper wires, one sensor's leads not draped over
  another's, avoids accidental shorts across the rails. Spread the four
  sensors' rail taps out along the board rather than crowding them into
  neighbouring columns — you will be re-seating these wires.
- **Watch for a split rail:** most full-size boards break each rail in the
  middle, near column 30. With four sensors spread along the board, an
  unbridged split leaves two of them dead while the other two work fine.
- **Floating OUT pin = fires randomly:** if a sensor triggers with no one near
  it, isolation-test by jumpering its OUT pin straight to GND — if it goes
  quiet, the code is fine and that sensor's OUT wire isn't seated properly.
- **One sensor at a time:** wire and verify each sensor alone in the Serial
  Monitor before adding the next. Four sensors wired all at once give you
  sixteen possible pin-to-room mix-ups and no way to tell which one you have.

---

## Verifying

Upload `arduino/motion_sensor/motion_sensor.ino`, open the Serial Monitor at
**9600**, and wait for `System Ready`. Then wave at one sensor at a time and
confirm you get the room you expect — not just *a* room:

```
ROOMC_MOTION_DETECTED     <- Pin 2
ROOMA_MOTION_DETECTED     <- Pin 3
ROOMB_MOTION_DETECTED     <- Pin 4
ROOMD_MOTION_DETECTED     <- Pin 5
```

Each is followed by `ROOMx_MOTION_STOPPED` about 2 seconds after the movement
ends. If waving at one room lights up another, two OUT wires are swapped —
move the wire, not the code.

Once all four are confirmed, the rest of the system already supports them:
`config.php` lists all four zones, both serial bridges match all four, and the
dashboard builds its zone cards from `ALLOWED_ZONES`.
