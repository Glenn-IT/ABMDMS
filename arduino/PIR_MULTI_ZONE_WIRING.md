# Multi-Zone PIR Wiring — Breadboard (3 sensors)

Adds **2 new PIR sensors** to the existing ABMDMS setup, for a total of
**3 zones**. Wiring only — no code changes here. Verify each sensor with
the Arduino IDE Serial Monitor before touching the PHP/DB/dashboard side.

| Zone | Sensor | Arduino pin | Status |
|------|--------|-------------|--------|
| Room C | PIR #1 (existing) | Digital Pin **2** | already wired, unchanged |
| Room A | PIR #2 (new) | Digital Pin **3** | new |
| Room B | PIR #3 (new) | Digital Pin **4** | new |

---

## Why a breadboard

An Arduino Uno has only **one 5V pin** and a couple of GND pins — not enough
to plug 3 sensors in directly. The breadboard's two power rails (`+` and `-`
running the length of the board) let all 3 PIR sensors share a single 5V/GND
feed from the Arduino, while each sensor's **OUT** wire still runs to its own
dedicated Arduino digital pin.

```
Arduino 5V  ──────► breadboard  +  rail (red)   ─── shared by all 3 sensors
Arduino GND ──────► breadboard  -  rail (blue)  ─── shared by all 3 sensors
Arduino Pin 2/3/4 ─► one dedicated wire per sensor OUT pin (not shared)
```

---

## ASCII wiring diagram

```
                         ARDUINO UNO
                    ┌───────────────────┐
                    │                   │
                    │  5V ●             │──────────────┐
                    │  GND ●            │───────────┐  │
                    │                   │           │  │
                    │  DIGITAL          │           │  │
                    │  PIN 2 ●──────────┼───────┐   │  │
                    │  PIN 3 ●──────────┼─────┐ │   │  │
                    │  PIN 4 ●──────────┼───┐ │ │   │  │
                    │                   │   │ │ │   │  │
                    └───────────────────┘   │ │ │   │  │
                                             │ │ │   │  │
    BREADBOARD                              │ │ │   │  │
    ┌─────────────────────────────────────┐ │ │ │   │  │
    │ (+) RED  RAIL ●───────────────────────┼─┼─┼───┼──┘  <- fed by Arduino 5V
    ├───────────────────────────────────────┼─┼─┼───┼──
    │ (-) BLUE RAIL ●───────────────────────┼─┼─┼───┘     <- fed by Arduino GND
    └───────────────────────────────────────┼─┼─┼─────────
                                             │ │ │
       PIR #1 "Room C"    PIR #2 "Room A"    │ │ │  PIR #3 "Room B"
       (existing, Pin 2)  (new, Pin 3)       │ │ │  (new, Pin 4)
        ┌──────────┐       ┌──────────┐      │ │ │   ┌──────────┐
        │  ╭────╮  │       │  ╭────╮  │      │ │ │   │  ╭────╮  │
        │  │dome│  │       │  │dome│  │      │ │ │   │  │dome│  │
        │  ╰────╯  │       │  ╰────╯  │      │ │ │   │  ╰────╯  │
        │ V  O  G  │       │ V  O  G  │      │ │ │   │ V  O  G  │
        └─┬──┬──┬──┘       └─┬──┬──┬──┘      │ │ │   └─┬──┬──┬──┘
          │  │  │            │  │  │         │ │ │     │  │  │
          │  │  └─► (-) rail │  │  └────────► │ │ │     │  │  └──► (-) rail
          │  └────► Pin 2 ───┼──┼──────────── ┘ │ │     │  └─────► Pin 4 ──┘
          └───────► (+) rail │  └────► Pin 3 ────┼─┘     └────────► (+) rail
                             └────────► (+) rail ┘
```

*(V = VCC, O = OUT, G = GND — but always confirm the actual pin order printed
under YOUR sensor's dome; it varies between HC-SR501 batches.)*

**Plain description of every wire, if the ASCII art is hard to follow:**

1. Arduino `5V` → breadboard `(+)` red rail
2. Arduino `GND` → breadboard `(-)` blue rail
3. **PIR #1 (Room C, existing):** VCC → `(+)` rail · GND → `(-)` rail · OUT → Arduino **Pin 2**
4. **PIR #2 (Room A, new):** VCC → `(+)` rail · GND → `(-)` rail · OUT → Arduino **Pin 3**
5. **PIR #3 (Room B, new):** VCC → `(+)` rail · GND → `(-)` rail · OUT → Arduino **Pin 4**

---

## Build notes (carried over from lessons already learned on PIR #1)

- **Warm-up:** every PIR needs ~30–60s to stabilize after power-on. With 3
  sensors on the same breadboard, stay away from **all of them** during
  warm-up — motion or body heat near any sensor can throw off its baseline.
- **Pin order varies:** read the `V / O / G` (or similar) labels printed
  directly under each sensor's dome before wiring — don't assume it matches
  a diagram or a different sensor you've used before.
- **Keep rails tidy:** short jumper wires, one sensor's leads not draped
  over another's, avoids accidental shorts across the rails.
- **Floating OUT pin = fires randomly:** if a sensor triggers with no one
  near it, isolation-test by jumpering its OUT pin straight to GND — if it
  goes quiet, the code is fine and that sensor's OUT wire isn't seated
  properly.
- **One sensor at a time:** wire and verify PIR #2 (Room A) alone in the
  Serial Monitor before adding PIR #3 (Room B) — easier to tell which
  sensor is misbehaving if something looks wrong.

---

## What's next

Once all 3 sensors are wired and confirmed quiet-at-rest / triggering
correctly in the Serial Monitor, the next step is updating
`arduino/motion_sensor/motion_sensor.ino` to read all 3 pins and print a
per-zone event (e.g. `ROOMA_MOTION_DETECTED`), followed by the DB/API/
dashboard changes to support multiple zones — not part of this step.
