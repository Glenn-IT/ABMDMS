/*
  ============================================================
  ABMDMS - Arduino Based Motion Detection Monitoring System
  File   : motion_sensor.ino
  Board  : Arduino Uno
  Sensor : 4x HC-SR501 PIR Motion Sensors (multi-zone)
  GSM    : SIM800L EVB (sends an SMS alert when motion starts)
  ============================================================

  WIRING - PIR SENSORS  (see arduino/PIR_MULTI_ZONE_WIRING.md)
  ------
  All 4 PIR sensors share the breadboard's 5V (+) and GND (-) rails.
  Each sensor's OUT wire goes to its OWN Arduino digital pin:

  Room C (existing) PIR OUT  ->  Arduino Digital Pin 2
  Room A (new)      PIR OUT  ->  Arduino Digital Pin 3
  Room B (new)      PIR OUT  ->  Arduino Digital Pin 4
  Room D (new)      PIR OUT  ->  Arduino Digital Pin 5
  All 4 PIR VCC               ->  breadboard (+) rail -> Arduino 5V
  All 4 PIR GND               ->  breadboard (-) rail -> Arduino GND

  WIRING - SIM800L V2.2 (UNV, the 5V board)  (see arduino/SIM800L_WIRING.md - READ IT FIRST)
  ------
  SIM800L TXD  ->  Arduino Pin 10          (direct)
  SIM800L RXD  ->  Arduino Pin 11          (direct - V2.2 is 5V-logic, no divider)
  SIM800L RST  ->  Arduino Pin 12          (optional)
  SIM800L 5Vin ->  power bank 5V           (direct - NEVER the Arduino 5V pin or DC jack)
  SIM800L GND  ->  power bank GND AND Arduino GND (must be common)
  SIM800L VDD  ->  leave UNCONNECTED       (2.8V reference output, not a power pin)
  1000uF capacitor across the module's 5Vin / GND.
  NOTE: the bare 3.7-4.2V SIM800L needs a buck + a 1k/2k divider instead - see the wiring doc.

  WHAT THIS PROGRAM DOES
  ----------------------
  1. Waits for all 4 PIR sensors to warm up (they need time after power on).
  2. Watches Pins 2, 3, 4, and 5 for movement, independently per zone.
  3. Prints "<ZONE>_MOTION_DETECTED" one time when movement STARTS in a zone.
  4. Prints "<ZONE>_MOTION_STOPPED"  one time when movement ENDS in a zone.
     ZONE is one of: ROOMC (Pin 2), ROOMA (Pin 3), ROOMB (Pin 4), ROOMD (Pin 5).
  5. Sends an SMS through the SIM800L when motion STARTS (not when it stops),
     then prints "SMS_SENT:<ZONE>" / "SMS_FAIL:<ZONE>:<REASON>" /
     "SMS_SKIP:<ZONE>:COOLDOWN" so the laptop can log it too.

  IMPORTANT: each zone only prints when ITS OWN state CHANGES.
  If it printed on every loop, it would send thousands of
  messages per second and flood the database. Zones are tracked
  independently so triggering one zone never triggers another.

  IMPORTANT: the SMS code NEVER uses delay(). Sending one text takes
  5-10 seconds of back-and-forth with the module. If we waited for it,
  the Arduino would stop watching the PIR sensors for that whole time
  and miss real movement. Instead the send is split into small steps
  ("states") and loop() checks on it a little bit at a time.

  Open Tools > Serial Monitor and set the baud rate to 9600.
*/

#include <SoftwareSerial.h>


// ============================================================
// SECTION 1 - SETTINGS
// You can change these numbers if you need to.
// ============================================================

const int NUM_ZONES = 4;
const int PIR_PIN[NUM_ZONES]   = { 2,       3,       4,       5       }; // OUT wire per zone
const char* ZONE_NAME[NUM_ZONES] = { "ROOMC", "ROOMA", "ROOMB", "ROOMD" }; // printed in event tokens
const char* ZONE_TEXT[NUM_ZONES] = { "Room C", "Room A", "Room B", "Room D" }; // written inside the SMS

const int LED_PIN = 13;         // Built-in LED on the Arduino board (lights when ANY zone is active)

const unsigned long WARMUP_SECONDS   = 30;   // PIR warm-up time in seconds
const unsigned long BAUD_RATE        = 9600; // Must match the Serial Monitor
const unsigned long STOP_CONFIRM_MS  = 2000; // Wait this long before saying motion stopped


// ------------------------------------------------------------
// SMS / SIM800L SETTINGS
// ------------------------------------------------------------

// >>> PUT YOUR OWN PHONE NUMBER HERE, in international format. <<<
// Philippines example: 0917 123 4567  ->  "+639171234567"
const char* SMS_RECIPIENT = "+639169751409";

// Set this to false to run the system with NO GSM module attached
// (everything else still works exactly like before).
const bool SIM_ENABLED = true;

const int SIM_RX_PIN  = 10;   // Arduino Pin 10 <- SIM800L TXD  (direct)
const int SIM_TX_PIN  = 11;   // Arduino Pin 11 -> SIM800L RXD  (via 1k/2k divider)
const int SIM_RST_PIN = 12;   // Arduino Pin 12 -> SIM800L RST  (optional)

const unsigned long SIM_BAUD_RATE   = 9600;   // SIM800L default speed

// Per zone: never send more than one SMS inside this window.
// This is what stops one person walking around from draining your load.
const unsigned long SMS_COOLDOWN_MS = 60000;  // 60 seconds

const unsigned long SMS_MIN_GAP_MS  = 5000;   // rest between any two sends
const unsigned long AT_TIMEOUT_MS   = 10000;  // wait for a normal AT reply
const unsigned long CMGS_TIMEOUT_MS = 30000;  // wait for the network to accept the SMS
const int SMS_MAX_FAILS_BEFORE_RESET = 3;     // hard-reset the module after this many fails


// ============================================================
// SECTION 2 - MEMORY (variables that remember things)
// ============================================================

bool motionActive[NUM_ZONES]      = { false, false, false, false }; // per-zone: already reported motion?
unsigned long lowStartedAt[NUM_ZONES] = { 0, 0, 0, 0 };          // per-zone: when it first went quiet

// --- SMS memory ---
SoftwareSerial sim(SIM_RX_PIN, SIM_TX_PIN);

bool          smsPending[NUM_ZONES]  = { false, false, false, false }; // zone is waiting for its text
unsigned long lastSmsAt[NUM_ZONES]   = { 0, 0, 0, 0 };                 // when this zone last texted
bool          smsEverSent[NUM_ZONES] = { false, false, false, false }; // has this zone EVER texted?

bool simReady   = false;   // did the module answer at start-up?
int  smsFailRun = 0;       // how many sends failed in a row

// The steps of one SMS send. loop() moves through these one at a time.
enum SmsState {
  SMS_IDLE,          // nothing to do
  SMS_WAIT_PROMPT,   // sent AT+CMGS="...", waiting for the ">" prompt
  SMS_WAIT_CONFIRM   // sent the message text, waiting for "+CMGS:"
};

SmsState      smsState        = SMS_IDLE;
int           smsZone         = -1;   // which zone we are texting about right now
unsigned long smsStateSince   = 0;    // when the current step started
unsigned long smsLastFinished = 0;    // when the last send ended (for SMS_MIN_GAP_MS)

// A small box to collect whatever the SIM800L says back to us.
const int SIM_BUF_SIZE = 64;
char simBuf[SIM_BUF_SIZE];
int  simBufLen = 0;


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

  Serial.println("ABMDMS - Multi-Zone Motion Detection System + SMS Alerts");

  // --- Wake up the GSM module ---
  // We do this FIRST, because the module also needs time to find
  // the network - it can do that while the PIR sensors warm up.
  simSetup();

  // --- PIR warm-up ---
  // The HC-SR501 gives false readings for the first few seconds
  // after power on. We wait and show a countdown so the user knows
  // the system is not frozen. All 4 sensors share one warm-up timer,
  // so stay away from ALL of them until it finishes.
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

        // Ask for an SMS about this zone. It is NOT sent here - it
        // is put in line and sent a small piece at a time by smsTick().
        queueSms(i);
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
          // NOTE: no SMS here on purpose. Only the START of motion texts.
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

  // Move any in-progress SMS forward by one small step.
  // This returns immediately - it never waits.
  smsTick();

  // Small pause so we do not read the pins millions of times per second
  delay(50);
}


// ============================================================
// SECTION 5 - SMS: PUTTING A ZONE IN LINE
// ============================================================

// Called the moment a zone starts seeing movement.
void queueSms(int zone) {

  if (!SIM_ENABLED) {
    return;
  }

  // Cooldown check: has this zone texted recently?
  // smsEverSent tells us the difference between "texted a long time
  // ago" and "never texted at all" (millis() starts at 0, so without
  // this flag the very first alert of every zone would be blocked).
  if (smsEverSent[zone] && (millis() - lastSmsAt[zone] < SMS_COOLDOWN_MS)) {
    Serial.print("SMS_SKIP:");
    Serial.print(ZONE_NAME[zone]);
    Serial.println(":COOLDOWN");
    return;
  }

  if (!simReady) {
    Serial.print("SMS_FAIL:");
    Serial.print(ZONE_NAME[zone]);
    Serial.println(":NOMODULE");
    return;
  }

  smsPending[zone] = true;
}


// ============================================================
// SECTION 6 - SMS: THE STEP-BY-STEP SENDER
// This is called once per loop(). It does a tiny bit of work
// and returns straight away, so motion detection never pauses.
// ============================================================

void smsTick() {

  if (!SIM_ENABLED) {
    return;
  }

  switch (smsState) {

    // --------------------------------------------------------
    // STEP 0 - Nothing being sent. Is anyone waiting?
    // --------------------------------------------------------
    case SMS_IDLE: {

      if (!simReady) {
        return;
      }

      // Give the module a short rest between messages
      if (smsLastFinished != 0 && (millis() - smsLastFinished < SMS_MIN_GAP_MS)) {
        return;
      }

      // Find the first zone waiting for a text
      int zone = -1;
      for (int i = 0; i < NUM_ZONES; i++) {
        if (smsPending[i]) {
          zone = i;
          break;
        }
      }
      if (zone < 0) {
        return;   // nobody waiting
      }

      smsPending[zone] = false;
      smsZone          = zone;

      // Tell the module who we are texting. It should answer ">".
      simBufClear();
      sim.print("AT+CMGS=\"");
      sim.print(SMS_RECIPIENT);
      sim.print("\"\r");

      smsState      = SMS_WAIT_PROMPT;
      smsStateSince = millis();
      break;
    }


    // --------------------------------------------------------
    // STEP 1 - Waiting for the ">" prompt
    // --------------------------------------------------------
    case SMS_WAIT_PROMPT: {

      simDrain();

      if (simSaw(">")) {
        // The module is ready for the message text.
        sim.print("ABMDMS ALERT: Motion detected in ");
        sim.print(ZONE_TEXT[smsZone]);
        sim.print(" (");
        sim.print(ZONE_NAME[smsZone]);
        sim.print("). Uptime ");
        sim.print(millis() / 60000UL);
        sim.print(" min.");
        sim.write(26);          // Ctrl+Z = "that is the whole message, send it"

        simBufClear();
        smsState      = SMS_WAIT_CONFIRM;
        smsStateSince = millis();
      }
      else if (simSaw("ERROR")) {
        smsFinish(false, "ERROR");
      }
      else if (millis() - smsStateSince >= AT_TIMEOUT_MS) {
        smsFinish(false, "NOPROMPT");
      }
      break;
    }


    // --------------------------------------------------------
    // STEP 2 - Waiting for the network to accept the message
    // --------------------------------------------------------
    case SMS_WAIT_CONFIRM: {

      simDrain();

      if (simSaw("+CMGS")) {
        smsFinish(true, "");
      }
      else if (simSaw("ERROR")) {
        smsFinish(false, "SENDFAIL");
      }
      else if (millis() - smsStateSince >= CMGS_TIMEOUT_MS) {
        smsFinish(false, "TIMEOUT");
      }
      break;
    }
  }
}


// Ends the current send, prints the result line for the laptop,
// and starts the zone's cooldown either way (a failing module
// must not be hammered with a retry on every single movement).
void smsFinish(bool ok, const char* reason) {

  if (smsZone >= 0) {
    lastSmsAt[smsZone]   = millis();
    smsEverSent[smsZone] = true;

    if (ok) {
      Serial.print("SMS_SENT:");
      Serial.println(ZONE_NAME[smsZone]);
    } else {
      Serial.print("SMS_FAIL:");
      Serial.print(ZONE_NAME[smsZone]);
      Serial.print(":");
      Serial.println(reason);
    }
  }

  smsFailRun = ok ? 0 : (smsFailRun + 1);

  smsZone         = -1;
  smsState        = SMS_IDLE;
  smsLastFinished = millis();
  simBufClear();

  // Too many failures in a row usually means the module is stuck.
  if (smsFailRun >= SMS_MAX_FAILS_BEFORE_RESET) {
    smsFailRun = 0;
    simReset();
  }
}


// ============================================================
// SECTION 7 - SMS: TALKING TO THE MODULE
// ============================================================

// Empty the collection box.
void simBufClear() {
  simBufLen = 0;
  simBuf[0] = '\0';
}


// Copy whatever the module has said into the box.
// Never waits - if there is nothing to read it returns at once.
void simDrain() {
  while (sim.available()) {
    char c = (char) sim.read();

    if (c == '\0') {
      continue;
    }

    // Box full? Throw away the oldest half and keep going.
    if (simBufLen >= SIM_BUF_SIZE - 1) {
      int keep = SIM_BUF_SIZE / 2;
      memmove(simBuf, simBuf + (simBufLen - keep), keep);
      simBufLen = keep;
    }

    simBuf[simBufLen++] = c;
    simBuf[simBufLen]   = '\0';
  }
}


// Did the module say this word anywhere in what we collected?
// We search for a PIECE of text (not an exact match) on purpose:
// SoftwareSerial sometimes drops a character, and an exact match
// would then fail a send that actually worked.
bool simSaw(const char* token) {
  return strstr(simBuf, token) != NULL;
}


// Send one AT command and wait for the expected reply.
// This one IS allowed to wait, because it is only used in setup(),
// before the system starts watching for motion.
bool simCommand(const char* command, const char* expect, unsigned long timeoutMs) {

  simBufClear();
  sim.print(command);
  sim.print("\r");

  unsigned long startedAt = millis();

  while (millis() - startedAt < timeoutMs) {
    simDrain();
    if (simSaw(expect)) {
      return true;
    }
    if (simSaw("ERROR")) {
      return false;
    }
  }
  return false;
}


// Wake the module up and check it can actually text.
// Prints SIM_READY or SIM_FAIL:<reason> for the serial bridge.
void simSetup() {

  if (!SIM_ENABLED) {
    Serial.println("SIM_FAIL:DISABLED");
    return;
  }

  pinMode(SIM_RST_PIN, OUTPUT);
  digitalWrite(SIM_RST_PIN, HIGH);   // RST is active LOW - keep it released

  sim.begin(SIM_BAUD_RATE);
  Serial.println("Starting SIM800L, please wait...");
  delay(3000);                       // the module needs a moment after power on

  // 1. Is it alive?  ->  "OK"
  if (!simCommand("AT", "OK", 5000)) {
    Serial.println("SIM_FAIL:NOREPLY");
    Serial.println("   Check: external power, common GND, TX/RX not swapped.");
    simReady = false;
    return;
  }

  // 2. Is the SIM card in and unlocked?  ->  "READY"
  if (!simCommand("AT+CPIN?", "READY", 5000)) {
    Serial.println("SIM_FAIL:NOSIM");
    Serial.println("   Check: SIM inserted properly, PIN lock turned OFF.");
    simReady = false;
    return;
  }

  // 3. How strong is the signal?  (99 = no signal at all)
  simCommand("AT+CSQ", "+CSQ", 5000);
  Serial.print("   Signal: ");
  Serial.println(simBuf);

  // 4. Did it join the network?  ->  ",1" (home) or ",5" (roaming)
  bool registered = simCommand("AT+CREG?", ",1", 10000);
  if (!registered) {
    registered = simCommand("AT+CREG?", ",5", 10000);
  }
  if (!registered) {
    Serial.println("SIM_FAIL:NONETWORK");
    Serial.println("   Check: antenna, load/credit, and 2G coverage in this area.");
    simReady = false;
    return;
  }

  // 5. Use plain text messages (not the binary PDU format)
  if (!simCommand("AT+CMGF=1", "OK", 5000)) {
    Serial.println("SIM_FAIL:TEXTMODE");
    simReady = false;
    return;
  }

  simReady = true;
  Serial.println("SIM_READY");
  Serial.print("   Alerts will be sent to: ");
  Serial.println(SMS_RECIPIENT);
}


// Hard-reset the module by pulling its RST pin low for a moment.
// Used when several sends fail in a row.
void simReset() {
  Serial.println("SIM_RESET");

  digitalWrite(SIM_RST_PIN, LOW);
  delay(150);
  digitalWrite(SIM_RST_PIN, HIGH);

  // The module is unusable while it restarts. We mark it as not
  // ready so no send is attempted; the next successful start-up
  // check would set it back. Keeping it simple: give it time and
  // re-run the same checks setup() used.
  delay(3000);
  simBufClear();

  simReady = simCommand("AT", "OK", 5000) && simCommand("AT+CMGF=1", "OK", 5000);

  Serial.println(simReady ? "SIM_READY" : "SIM_FAIL:RESETFAILED");
}
