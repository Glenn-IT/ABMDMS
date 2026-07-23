/*
  ============================================================
  ABMDMS - Arduino Based Motion Detection Monitoring System
  File   : motion_sensor.ino
  Board  : Arduino Uno
  Sensor : HC-SR501 PIR Motion Sensor
  ============================================================

  WIRING
  ------
  PIR VCC  ->  Arduino 5V
  PIR GND  ->  Arduino GND
  PIR OUT  ->  Arduino Digital Pin 2

  WHAT THIS PROGRAM DOES
  ----------------------
  1. Waits for the PIR sensor to warm up (it needs time after power on).
  2. Watches Digital Pin 2 for movement.
  3. Prints "MOTION_DETECTED" one time when movement STARTS.
  4. Prints "MOTION_STOPPED"  one time when movement ENDS.

  IMPORTANT: it only prints when the state CHANGES.
  If it printed on every loop, it would send thousands of
  messages per second and flood the database.

  Open Tools > Serial Monitor and set the baud rate to 9600.
*/


// ============================================================
// SECTION 1 - SETTINGS
// You can change these numbers if you need to.
// ============================================================

const int PIR_PIN = 2;          // PIR OUT wire is connected to Digital Pin 2
const int LED_PIN = 13;         // Built-in LED on the Arduino board

const unsigned long WARMUP_SECONDS   = 30;   // PIR warm-up time in seconds
const unsigned long BAUD_RATE        = 9600; // Must match the Serial Monitor
const unsigned long STOP_CONFIRM_MS  = 2000; // Wait this long before saying motion stopped


// ============================================================
// SECTION 2 - MEMORY (variables that remember things)
// ============================================================

bool motionActive = false;      // true = we already reported motion is happening
unsigned long lowStartedAt = 0; // when the sensor first went quiet


// ============================================================
// SECTION 3 - SETUP
// Runs ONE TIME when the Arduino is powered on or reset.
// ============================================================

void setup() {
  // Start the USB serial connection to the laptop
  Serial.begin(BAUD_RATE);

  // Tell the Arduino which pins are inputs and outputs
  pinMode(PIR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // --- PIR warm-up ---
  // The HC-SR501 gives false readings for the first few seconds
  // after power on. We wait and show a countdown so the user knows
  // the system is not frozen.
  Serial.println("ABMDMS - Motion Detection System");
  Serial.print("Warming up PIR sensor, please stay still (");
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

  // Read the sensor. HIGH = movement, LOW = no movement.
  int sensorValue = digitalRead(PIR_PIN);


  // --------------------------------------------------------
  // CASE A: The sensor sees movement
  // --------------------------------------------------------
  if (sensorValue == HIGH) {

    // Movement is still happening, so cancel any "stop" countdown
    lowStartedAt = 0;

    // Only announce it if we have NOT already announced it.
    // This is what stops duplicate messages.
    if (motionActive == false) {
      motionActive = true;
      digitalWrite(LED_PIN, HIGH);   // turn the board LED on

      Serial.println("MOTION_DETECTED");   // <-- the laptop reads this line
    }
  }


  // --------------------------------------------------------
  // CASE B: The sensor sees nothing
  // --------------------------------------------------------
  else {

    // Only care about this if motion was previously happening
    if (motionActive == true) {

      // Start a small timer the first moment it goes quiet.
      // The PIR output can flicker for a moment, so we wait a
      // couple of seconds to be sure the person really left.
      if (lowStartedAt == 0) {
        lowStartedAt = millis();
      }

      // Has it stayed quiet long enough?
      if (millis() - lowStartedAt >= STOP_CONFIRM_MS) {
        motionActive = false;
        lowStartedAt = 0;
        digitalWrite(LED_PIN, LOW);    // turn the board LED off

        Serial.println("MOTION_STOPPED");  // <-- the laptop reads this line
      }
    }
  }

  // Small pause so we do not read the pin millions of times per second
  delay(50);
}
