/*
  ============================================================
  ABMDMS - Arduino Based Motion Detection Monitoring System
  File   : motion_sensor.ino
  Board  : Arduino Uno
  Sensor : 4x HC-SR501 PIR Motion Sensors (multi-zone)
  ============================================================

  WIRING  (see arduino/PIR_MULTI_ZONE_WIRING.md for the diagram)
  ------
  All 4 PIR sensors share the breadboard's 5V (+) and GND (-) rails.
  Each sensor's OUT wire goes to its OWN Arduino digital pin:

  Room C (existing) PIR OUT  ->  Arduino Digital Pin 2
  Room A (new)      PIR OUT  ->  Arduino Digital Pin 3
  Room B (new)      PIR OUT  ->  Arduino Digital Pin 4
  Room D (new)      PIR OUT  ->  Arduino Digital Pin 5
  All 4 PIR VCC               ->  breadboard (+) rail -> Arduino 5V
  All 4 PIR GND               ->  breadboard (-) rail -> Arduino GND

  WHAT THIS PROGRAM DOES
  ----------------------
  1. Waits for all 4 PIR sensors to warm up (they need time after power on).
  2. Watches Pins 2, 3, 4, and 5 for movement, independently per zone.
  3. Prints "<ZONE>_MOTION_DETECTED" one time when movement STARTS in a zone.
  4. Prints "<ZONE>_MOTION_STOPPED"  one time when movement ENDS in a zone.
     ZONE is one of: ROOMC (Pin 2), ROOMA (Pin 3), ROOMB (Pin 4), ROOMD (Pin 5).

  IMPORTANT: each zone only prints when ITS OWN state CHANGES.
  If it printed on every loop, it would send thousands of
  messages per second and flood the database. Zones are tracked
  independently so triggering one zone never triggers another.

  Open Tools > Serial Monitor and set the baud rate to 9600.
*/


// ============================================================
// SECTION 1 - SETTINGS
// You can change these numbers if you need to.
// ============================================================

const int NUM_ZONES = 4;
const int PIR_PIN[NUM_ZONES]   = { 2,       3,       4,       5       }; // OUT wire per zone
const char* ZONE_NAME[NUM_ZONES] = { "ROOMC", "ROOMA", "ROOMB", "ROOMD" }; // printed in event tokens

const int LED_PIN = 13;         // Built-in LED on the Arduino board (lights when ANY zone is active)

const unsigned long WARMUP_SECONDS   = 30;   // PIR warm-up time in seconds
const unsigned long BAUD_RATE        = 9600; // Must match the Serial Monitor
const unsigned long STOP_CONFIRM_MS  = 2000; // Wait this long before saying motion stopped


// ============================================================
// SECTION 2 - MEMORY (variables that remember things)
// ============================================================

bool motionActive[NUM_ZONES]      = { false, false, false, false }; // per-zone: already reported motion?
unsigned long lowStartedAt[NUM_ZONES] = { 0, 0, 0, 0 };          // per-zone: when it first went quiet


// ============================================================
// SECTION 3 - SETUP
// Runs ONE TIME when the Arduino is powered on or reset.
// ============================================================

void setup() {
  // Start the USB serial connection to the laptop
  Serial.begin(BAUD_RATE);

  // Tell the Arduino which pins are inputs and outputs
  for (int i = 0; i < NUM_ZONES; i++) {
    pinMode(PIR_PIN[i], INPUT);
  }
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // --- PIR warm-up ---
  // The HC-SR501 gives false readings for the first few seconds
  // after power on. We wait and show a countdown so the user knows
  // the system is not frozen. All 4 sensors share one warm-up timer,
  // so stay away from ALL of them until it finishes.
  Serial.println("ABMDMS - Multi-Zone Motion Detection System");
  Serial.print("Warming up ");
  Serial.print(NUM_ZONES);
  Serial.print(" PIR sensors, please stay still (");
  Serial.print(WARMUP_SECONDS);
  Serial.println(" seconds)...");

  for (unsigned long i = WARMUP_SECONDS; i > 0; i--) {
    Serial.print(i);
    Serial.println("...");
    delay(1000);   // wait 1 second
  }

  // Ready message
  Serial.println("System Ready");
  Serial.println("Waiting for motion...");
}


// ============================================================
// SECTION 4 - MAIN LOOP
// Runs over and over again, forever.
// ============================================================

void loop() {

  // Check each zone independently, one after another.
  for (int i = 0; i < NUM_ZONES; i++) {

    // Read this zone's sensor. HIGH = movement, LOW = no movement.
    int sensorValue = digitalRead(PIR_PIN[i]);


    // --------------------------------------------------------
    // CASE A: This zone's sensor sees movement
    // --------------------------------------------------------
    if (sensorValue == HIGH) {

      // Movement is still happening, so cancel any "stop" countdown
      lowStartedAt[i] = 0;

      // Only announce it if we have NOT already announced it.
      // This is what stops duplicate messages.
      if (motionActive[i] == false) {
        motionActive[i] = true;
        digitalWrite(LED_PIN, HIGH);   // turn the board LED on (any zone active)

        Serial.print(ZONE_NAME[i]);
        Serial.println("_MOTION_DETECTED");   // <-- the laptop reads this line
      }
    }


    // --------------------------------------------------------
    // CASE B: This zone's sensor sees nothing
    // --------------------------------------------------------
    else {

      // Only care about this if motion was previously happening
      if (motionActive[i] == true) {

        // Start a small timer the first moment it goes quiet.
        // The PIR output can flicker for a moment, so we wait a
        // couple of seconds to be sure the person really left.
        if (lowStartedAt[i] == 0) {
          lowStartedAt[i] = millis();
        }

        // Has it stayed quiet long enough?
        if (millis() - lowStartedAt[i] >= STOP_CONFIRM_MS) {
          motionActive[i] = false;
          lowStartedAt[i] = 0;

          Serial.print(ZONE_NAME[i]);
          Serial.println("_MOTION_STOPPED");  // <-- the laptop reads this line
        }
      }
    }
  }

  // The board LED reflects whether ANY zone is currently active
  bool anyActive = false;
  for (int i = 0; i < NUM_ZONES; i++) {
    if (motionActive[i]) anyActive = true;
  }
  digitalWrite(LED_PIN, anyActive ? HIGH : LOW);

  // Small pause so we do not read the pins millions of times per second
  delay(50);
}
