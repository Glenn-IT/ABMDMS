/*
  ============================================================
  PIR + SMS TEST RIG
  File   : pir_sms.ino
  Board  : Arduino Uno
  Sensor : 1x HC-SR501 PIR Motion Sensor  (Pin 2, zone ROOM1)
  GSM    : SIM800L V2.2 (UNV, the 5V board) - texts on motion
  ============================================================

  This is the SMALL version of the main ABMDMS sketch: one
  sensor instead of four, so you can prove the whole chain

      PIR -> Arduino -> SIM800L -> your phone
                     -> USB -> PowerShell -> PHP -> MySQL -> dashboard

  works, before wiring up the full four-room system.


  WIRING - PIR SENSOR
  -------------------
  PIR OUT  ->  Arduino Digital Pin 2
  PIR VCC  ->  breadboard (+) rail  ->  Arduino 5V
  PIR GND  ->  breadboard (-) rail  ->  Arduino GND

  WIRING - SIM800L V2.2  (see wiring.html in this folder)
  ---------------------
  SIM800L TXD  ->  Arduino Pin 10       (direct)
  SIM800L RXD  ->  Arduino Pin 11       (direct - V2.2 is a 5V-logic
                                         board, so NO 1k/2k divider)
  SIM800L RST  ->  Arduino Pin 12       (optional, used to recover)
  SIM800L 5Vin ->  external 5V supply + (NEVER the Arduino 5V pin -
                                         transmit bursts pull ~2A)
  SIM800L GND  ->  external supply - AND Arduino GND (must be common)
  SIM800L VDD  ->  leave UNCONNECTED    (it is a 2.8V reference
                                         OUTPUT, not a power input.
                                         Feeding it 5V kills the board.)
  1000uF capacitor across the module's 5Vin / GND, in PARALLEL.
  Watch the polarity: the striped leg goes to GND.


  WHAT THIS PROGRAM DOES
  ----------------------
  1. Wakes the SIM800L and checks it can actually text.
  2. Waits 30 seconds for the PIR to warm up (it lies at first).
  3. Prints "ROOM1_MOTION_DETECTED" once when movement STARTS.
  4. Prints "ROOM1_MOTION_STOPPED"  once when movement ENDS.
  5. Texts your phone when movement STARTS (never when it stops),
     then prints "SMS_SENT:ROOM1" / "SMS_FAIL:ROOM1:<REASON>" /
     "SMS_SKIP:ROOM1:COOLDOWN" so the laptop can log the result.

  IMPORTANT: it only prints when the state CHANGES. Printing every
  loop would flood the database with thousands of rows per second.

  IMPORTANT: the SMS code NEVER uses delay(). One text takes 5-10
  seconds of back-and-forth with the module. If we waited for it,
  the Arduino would stop watching the sensor for that whole time
  and miss real movement. The send is split into small steps
  ("states") and loop() nudges it forward a little at a time.

  Open Tools > Serial Monitor and set the baud rate to 9600.
*/

#include <SoftwareSerial.h>


// ============================================================
// SECTION 1 - SETTINGS
// ============================================================

const int PIR_PIN = 2;               // The PIR's OUT wire
const int LED_PIN = 13;              // Built-in LED, lights while motion is active

const char* ZONE_NAME = "ROOM1";     // Printed in the event tokens
const char* ZONE_TEXT = "Room 1";    // Written inside the SMS

const unsigned long WARMUP_SECONDS  = 30;    // PIR warm-up time
const unsigned long BAUD_RATE       = 9600;  // Must match the Serial Monitor
const unsigned long STOP_CONFIRM_MS = 2000;  // Quiet for this long = motion really stopped


// ------------------------------------------------------------
// SMS / SIM800L SETTINGS
// ------------------------------------------------------------

// >>> PUT YOUR OWN PHONE NUMBER HERE, in international format. <<<
// Philippines example: 0917 123 4567  ->  "+639171234567"
const char* SMS_RECIPIENT = "+639169751409";

// Set this to false to test the PIR half on its own, with no
// GSM module attached. Everything else still works.
const bool SIM_ENABLED = true;

const int SIM_RX_PIN  = 10;   // Arduino Pin 10 <- SIM800L TXD  (direct)
const int SIM_TX_PIN  = 11;   // Arduino Pin 11 -> SIM800L RXD  (direct on V2.2)
const int SIM_RST_PIN = 12;   // Arduino Pin 12 -> SIM800L RST  (active LOW)

const unsigned long SIM_BAUD_RATE   = 9600;   // SIM800L default speed

// Never send more than one SMS inside this window. This is what
// stops one person walking around from draining your load.
const unsigned long SMS_COOLDOWN_MS = 60000;  // 60 seconds

// Rest between any two sends. The module needs a moment to finish
// tidying up after one message before it will accept the next -
// without this, a failed send is followed instantly by another that
// fails for no reason except that it was too early.
const unsigned long SMS_MIN_GAP_MS  = 5000;   // 5 seconds

const unsigned long AT_TIMEOUT_MS   = 10000;  // wait for a normal AT reply
const unsigned long CMGS_TIMEOUT_MS = 30000;  // wait for the network to accept the SMS
const int SMS_MAX_FAILS_BEFORE_RESET = 3;     // hard-reset the module after this many fails
const int MIN_SIGNAL = 10;                    // below this, sending is unreliable


// ============================================================
// SECTION 2 - MEMORY (variables that remember things)
// ============================================================

bool          motionActive = false;   // have we already announced motion?
unsigned long lowStartedAt = 0;       // when the sensor first went quiet

// --- SMS memory ---
SoftwareSerial sim(SIM_RX_PIN, SIM_TX_PIN);

bool          smsPending  = false;    // a text is waiting to be sent
unsigned long lastSmsAt   = 0;        // when we last texted
bool          smsEverSent = false;    // have we EVER texted? (millis() starts at 0,
                                      // so without this the first alert is blocked)

bool simReady   = false;   // did the module answer at start-up?
int  smsFailRun = 0;       // how many sends failed in a row

// The steps of one SMS send. loop() moves through these one at a time.
enum SmsState {
  SMS_IDLE,          // nothing to do
  SMS_WAIT_PROMPT,   // sent AT+CMGS="...", waiting for the ">" prompt
  SMS_WAIT_CONFIRM   // sent the message text, waiting for "+CMGS:"
};

SmsState      smsState        = SMS_IDLE;
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
  Serial.begin(BAUD_RATE);

  pinMode(PIR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.println("PIR + SMS Test Rig - 1 sensor, 1 SIM800L");

  // Wake the GSM module FIRST. It needs time to find the network,
  // and it can do that while the PIR sensor warms up.
  simSetup();

  // The HC-SR501 gives false readings for the first few seconds
  // after power on. Wait it out, with a countdown so you can see
  // the system is not frozen. Stay away from the sensor until it
  // says "System Ready".
  Serial.print("Warming up the PIR sensor, please stay still (");
  Serial.print(WARMUP_SECONDS);
  Serial.println(" seconds)...");

  for (unsigned long i = WARMUP_SECONDS; i > 0; i--) {
    Serial.print(i);
    Serial.println("...");
    delay(1000);
  }

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


  // ----------------------------------------------------------
  // CASE A: the sensor sees movement
  // ----------------------------------------------------------
  if (sensorValue == HIGH) {

    // Movement is still happening, so cancel any "stop" countdown
    lowStartedAt = 0;

    // Only announce it if we have NOT already announced it.
    // This is what stops duplicate messages.
    if (motionActive == false) {
      motionActive = true;
      digitalWrite(LED_PIN, HIGH);

      Serial.print(ZONE_NAME);
      Serial.println("_MOTION_DETECTED");   // <-- the laptop reads this line

      // Ask for an SMS. It is NOT sent here - it is put in line
      // and sent a small piece at a time by smsTick().
      queueSms();
    }
  }


  // ----------------------------------------------------------
  // CASE B: the sensor sees nothing
  // ----------------------------------------------------------
  else {

    // Only care if motion was previously happening
    if (motionActive == true) {

      // Start a small timer the first moment it goes quiet. The
      // PIR output flickers, so wait a couple of seconds to be
      // sure the person really left.
      if (lowStartedAt == 0) {
        lowStartedAt = millis();
      }

      if (millis() - lowStartedAt >= STOP_CONFIRM_MS) {
        motionActive = false;
        lowStartedAt = 0;
        digitalWrite(LED_PIN, LOW);

        Serial.print(ZONE_NAME);
        Serial.println("_MOTION_STOPPED");  // <-- the laptop reads this line
        // NOTE: no SMS here on purpose. Only the START of motion texts.
      }
    }
  }

  // Move any in-progress SMS forward by one small step.
  // This returns immediately - it never waits.
  smsTick();

  // Small pause so we do not read the pin millions of times per second
  delay(50);
}


// ============================================================
// SECTION 5 - SMS: PUTTING A MESSAGE IN LINE
// ============================================================

// Called the moment the sensor starts seeing movement.
void queueSms() {

  if (!SIM_ENABLED) {
    return;
  }

  // Cooldown check: did we text recently?
  // smsEverSent tells the difference between "texted a long time
  // ago" and "never texted at all".
  if (smsEverSent && (millis() - lastSmsAt < SMS_COOLDOWN_MS)) {
    Serial.print("SMS_SKIP:");
    Serial.print(ZONE_NAME);
    Serial.println(":COOLDOWN");
    return;
  }

  if (!simReady) {
    Serial.print("SMS_FAIL:");
    Serial.print(ZONE_NAME);
    Serial.println(":NOMODULE");
    return;
  }

  smsPending = true;
}


// ============================================================
// SECTION 6 - SMS: THE STEP-BY-STEP SENDER
// Called once per loop(). Does a tiny bit of work and returns
// straight away, so motion detection never pauses.
// ============================================================

void smsTick() {

  if (!SIM_ENABLED) {
    return;
  }

  switch (smsState) {

    // --------------------------------------------------------
    // STEP 0 - Nothing being sent. Is anything waiting?
    // --------------------------------------------------------
    case SMS_IDLE: {

      if (!simReady || !smsPending) {
        return;
      }

      // Give the module its rest before starting another message.
      if (smsLastFinished != 0 && (millis() - smsLastFinished < SMS_MIN_GAP_MS)) {
        return;   // the request stays pending; we will come back to it
      }

      smsPending = false;

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
        sim.print(ZONE_TEXT);
        sim.print(" (");
        sim.print(ZONE_NAME);
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
// and starts the cooldown either way (a failing module must not
// be hammered with a retry on every single movement).
void smsFinish(bool ok, const char* reason) {

  // If we are giving up part-way through, the module may still be
  // sitting at its ">" prompt waiting for more message text. ESC
  // abandons that half-typed message. Without this the module stays
  // in text-entry mode and swallows the NEXT AT+CMGS, so one failure
  // turns into an endless run of them.
  if (!ok) {
    sim.write((char) 27);   // ESC = throw this message away
    sim.print("\r");
  }

  lastSmsAt   = millis();
  smsEverSent = true;

  if (ok) {
    Serial.print("SMS_SENT:");
    Serial.println(ZONE_NAME);
  } else {
    Serial.print("SMS_FAIL:");
    Serial.print(ZONE_NAME);
    Serial.print(":");
    Serial.println(reason);
  }

  smsFailRun = ok ? 0 : (smsFailRun + 1);

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


// Pull the number out of a "+CSQ: 18,0" reply.
// Returns -1 if there is no such reply in the buffer.
int parseCsq() {
  char* at = strstr(simBuf, "+CSQ:");
  if (at == NULL) {
    return -1;
  }
  return atoi(at + 5);   // atoi skips the space, stops at the comma
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

  // 2. Turn OFF command echo.
  //    By default the module repeats every command back to us before
  //    answering it. That echo lands in the same little buffer we
  //    search for replies, so "AT+CSQ" itself looks like a "+CSQ"
  //    answer and we read the question instead of the reading.
  //    ATE0 stops the echo and makes every check below trustworthy.
  simCommand("ATE0", "OK", 3000);

  // 3. Ask for real error numbers instead of a bare "ERROR",
  //    so a failure tells us WHY in the serial monitor.
  simCommand("AT+CMEE=2", "OK", 3000);

  // 4. Is the SIM card in and unlocked?  ->  "READY"
  if (!simCommand("AT+CPIN?", "READY", 5000)) {
    Serial.println("SIM_FAIL:NOSIM");
    Serial.println("   Check: SIM inserted properly, PIN lock turned OFF.");
    simReady = false;
    return;
  }

  // 5. How strong is the signal?
  //    0-31, bigger is better. 99 means "no signal at all".
  //    Under 10 is too weak to send reliably.
  simCommand("AT+CSQ", "+CSQ", 5000);
  int signal = parseCsq();
  Serial.print("   Signal: ");
  if (signal < 0) {
    Serial.println("could not read");
  } else if (signal == 99) {
    Serial.println("99 - NO SIGNAL (check the antenna)");
  } else {
    Serial.print(signal);
    Serial.println(signal < MIN_SIGNAL ? " - WEAK, sending may fail" : " - ok");
  }

  // 6. Did it join the network?  ->  ",1" (home) or ",5" (roaming)
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

  // 7. Use plain text messages (not the binary PDU format)
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

  // The module is unusable while it restarts, so give it time and
  // then re-run the checks that matter. A reset also forgets ATE0
  // and text mode, so both have to be set again.
  delay(3000);
  simBufClear();

  simReady = simCommand("AT", "OK", 5000);
  if (simReady) {
    simCommand("ATE0", "OK", 3000);
    simReady = simCommand("AT+CMGF=1", "OK", 5000);
  }

  // Start the rest timer from here too, so the first send after a
  // reset does not fire while the module is still waking up.
  smsLastFinished = millis();

  Serial.println(simReady ? "SIM_READY" : "SIM_FAIL:RESETFAILED");
}
