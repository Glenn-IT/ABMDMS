/*
  ============================================================
  RAW PIN MONITOR
  File   : pin_monitor.ino
  Board  : Arduino Uno
  ============================================================

  No zones. No SMS. No debounce. No cleverness at all.

  This just prints the raw electrical level of Pins 2, 3 and 4
  five times a second, so you can see with your own eyes what the
  Arduino is actually receiving.

  It exists to answer ONE question:

      "Are my sensors really all triggering at once, or is the
       four-zone sketch inventing it?"

  Because there is no logic here, whatever you see IS the hardware.
  If every column goes HIGH when you wave at one sensor, the
  sensors are all seeing you - no sketch can fix that.


  HOW TO READ IT
  --------------
      Pin2  Pin3  Pin4     <- ROOMC ROOMA ROOMB
      ----  ----  ----
       .     .     .       all quiet
       #     .     .       only Pin 2 sees motion  <- what you want
       #     #     #       they all see motion     <- the problem

  A '#' means HIGH (motion). A '.' means LOW (quiet).

  It only prints when something CHANGES, plus a heartbeat line
  every 5 seconds so you know it is still alive.


  WHAT TO DO WITH IT
  ------------------
  1. Upload. Wait for the 30-second warm-up.
  2. Stand well back. Every column should be '.' and stay there.
  3. Wave at ONE sensor. Watch which columns light up.

  They all light up  -> the sensors are physically seeing the same
                        movement. They are inches apart on one
                        breadboard and each has a ~110 degree cone
                        several metres deep. Separate them, or box
                        them in, or turn their sensitivity down.

  A column never lights, ever, no matter what you wave at
                     -> that sensor's OUT wire is not connected.

  A column copies another one exactly
                     -> those two OUT wires are shorted, or that
                        pin is floating and coupling to its neighbour.

  Open Tools > Serial Monitor and set the baud rate to 9600.
  ============================================================
*/

// Room D / Pin 5 is removed - the pin would not respond to two
// different sensors. To check it again later, set NUM_PINS to 4 and
// add 5 / "ROOMD" back to the two lists.
const int NUM_PINS = 3;
const int PINS[NUM_PINS]      = {  2,       3,       4      };
const char* ROOMS[NUM_PINS]   = { "ROOMC", "ROOMA", "ROOMB" };

const unsigned long WARMUP_SECONDS = 30;
const unsigned long HEARTBEAT_MS   = 5000;

int lastLevel[NUM_PINS];        // filled with -1 ("nothing read yet") in setup()
unsigned long lastPrint = 0;


void setup() {
  Serial.begin(9600);

  for (int i = 0; i < NUM_PINS; i++) {
    pinMode(PINS[i], INPUT);
    lastLevel[i] = -1;          // so the first reading always prints
  }

  Serial.println();
  Serial.println(F("================================================"));
  Serial.println(F("  RAW PIN MONITOR - no logic, just the wires"));
  Serial.println(F("================================================"));
  Serial.println(F("  '#' = HIGH (motion)     '.' = LOW (quiet)"));
  Serial.println();
  Serial.println(F("  Pin2  Pin3  Pin4"));
  Serial.println(F("  ROOMC ROOMA ROOMB"));
  Serial.println(F("  ----- ----- -----"));
  Serial.println();

  Serial.print(F("Warming up for "));
  Serial.print(WARMUP_SECONDS);
  Serial.println(F(" seconds. Stay away from ALL FOUR sensors."));

  for (unsigned long i = WARMUP_SECONDS; i > 0; i--) {
    Serial.print(i);
    Serial.println(F("..."));
    delay(1000);
  }

  Serial.println();
  Serial.println(F("Watching. Stand back first and check every column reads '.'"));
  Serial.println(F("Then wave at ONE sensor and see which columns react."));
  Serial.println();
}


void printRow(const char* tag) {
  Serial.print(F("  "));
  for (int i = 0; i < NUM_PINS; i++) {
    Serial.print(digitalRead(PINS[i]) == HIGH ? F("#") : F("."));
    Serial.print(F("     "));
  }
  Serial.print(F("  "));
  Serial.print(millis() / 1000);
  Serial.print(F("s  "));
  Serial.println(tag);
}


void loop() {

  // Did any pin change since last time?
  bool changed = false;
  for (int i = 0; i < NUM_PINS; i++) {
    int level = digitalRead(PINS[i]);
    if (level != lastLevel[i]) {
      lastLevel[i] = level;
      changed = true;
    }
  }

  if (changed) {
    // Name the rooms that are HIGH right now, so you do not have to
    // count columns while your hand is still in front of a sensor.
    Serial.print(F("  "));
    for (int i = 0; i < NUM_PINS; i++) {
      Serial.print(lastLevel[i] == HIGH ? F("#") : F("."));
      Serial.print(F("     "));
    }
    Serial.print(F("  "));
    Serial.print(millis() / 1000);
    Serial.print(F("s  "));

    bool any = false;
    for (int i = 0; i < NUM_PINS; i++) {
      if (lastLevel[i] == HIGH) {
        Serial.print(ROOMS[i]);
        Serial.print(F(" "));
        any = true;
      }
    }
    if (!any) {
      Serial.print(F("(all quiet)"));
    }
    Serial.println();

    lastPrint = millis();
  }

  // Heartbeat, so a silent screen never looks like a crash.
  if (millis() - lastPrint >= HEARTBEAT_MS) {
    printRow("(no change)");
    lastPrint = millis();
  }

  delay(50);
}
