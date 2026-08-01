/*
  ============================================================
  ONE-PIN TEST RIG          <<< TEST ONE SENSOR AT A TIME >>>
  File   : one_pin_test.ino
  Board  : Arduino Uno
  GSM    : SIM800L V2.2 (UNV, the 5V board) - texts on motion
  ============================================================

  This sketch watches ONE PIR sensor and ignores the other three.
  Use it to prove each sensor works, on its own, before running
  the real four-sensor sketch (pir_sms.ino).

  Why one at a time: four sensor s wired at once give you sixteen
  possible pin-to-room mix-ups and no way to tell which one you
  have. One at a time, a wrong answer points straight at one wire.


  HOW TO USE IT
  -------------
  1. Set TEST_PIN below to 2. Upload. Test that sensor.
  2. Change it to 3. Upload again. Test that sensor.
  3. Then 4, then 5.

  That is the ONLY line you change. The room name follows the pin
  automatically, so the two can never disagree.

  Every event line prints the pin AND the room, so if you forget to
  change the number you will see it immediately in the output.


  WIRING - unchanged from the full build. See wiring.html.
  -------
  PIR OUT  ->  Arduino Pin 2 / 3 / 4 / 5   (each its own wire)
  PIR VCC  ->  breadboard (+) rail  ->  Arduino 5V
  PIR GND  ->  breadboard (-) rail  ->  Arduino GND

  SIM800L TXD  ->  Arduino Pin 10       (direct)
  SIM800L RXD  ->  Arduino Pin 11       (direct - V2.2 is 5V logic,
                                         so NO 1k/2k divider)
  SIM800L RST  ->  Arduino Pin 12       (optional, used to recover)
  SIM800L 5Vin ->  external 5V supply + (NEVER the Arduino 5V pin)
  SIM800L GND  ->  external supply - AND Arduino GND (must be common)
  SIM800L VDD  ->  leave UNCONNECTED    (2.8V reference OUTPUT.
                                         Feeding it 5V kills the board.)
  1000uF capacitor across the module's 5Vin / GND, in PARALLEL.
  The striped leg goes to GND.

  You can leave all four sensors plugged in the whole time. This
  sketch simply does not look at the other three pins.

  Open Tools > Serial Monitor and set the baud rate to 9600.
  ============================================================
*/

#include <SoftwareSerial.h>


// ============================================================
// >>>>>>>>>>  THE ONLY LINE YOU CHANGE  <<<<<<<<<<
// ============================================================
//
//     2 = Room C
//     3 = Room A
//     4 = Room B
//
//     5 = Room D - REMOVED. The pin would not respond to two
//                  different sensors, so it is out of the system.
//                  Set TEST_PIN = 5 anyway if you want to retest it;
//                  the sketch still supports it and will tell you
//                  it is the retired pin.
//
// Change the number, press Upload, test that sensor. Then move on.

const int TEST_PIN = 2;

// ============================================================


// ------------------------------------------------------------
// SETTINGS
// ------------------------------------------------------------

const int LED_PIN = 13;              // Built-in LED, lights while motion is active

const unsigned long WARMUP_SECONDS  = 30;    // PIR warm-up time
const unsigned long BAUD_RATE       = 9600;  // Must match the Serial Monitor
const unsigned long STOP_CONFIRM_MS = 2000;  // Quiet for this long = motion really stopped

// After the warm-up, watch the sensor for this long without
// touching it. A healthy sensor sits perfectly still. See SECTION 4.
const unsigned long QUIET_TEST_MS   = 10000; // 10 seconds


// ------------------------------------------------------------
// SMS / SIM800L SETTINGS
// ------------------------------------------------------------

// >>> PUT YOUR OWN PHONE NUMBER HERE, in international format. <<<
const char* SMS_RECIPIENT = "+639169751409";

// Set to false to test the PIR half alone, with no GSM module
// attached. Useful if you just want to check the wiring quickly -
// it skips the ~15 second module start-up.
const bool SIM_ENABLED = true;

const int SIM_RX_PIN  = 10;   // Arduino Pin 10 <- SIM800L TXD  (direct)
const int SIM_TX_PIN  = 11;   // Arduino Pin 11 -> SIM800L RXD  (direct on V2.2)
const int SIM_RST_PIN = 12;   // Arduino Pin 12 -> SIM800L RST  (active LOW)

const unsigned long SIM_BAUD_RATE   = 9600;
const unsigned long SMS_COOLDOWN_MS = 60000;  // 60 s between texts
const unsigned long SMS_MIN_GAP_MS  = 5000;   // rest between any two sends
const unsigned long AT_TIMEOUT_MS   = 10000;
const unsigned long CMGS_TIMEOUT_MS = 30000;
const int SMS_MAX_FAILS_BEFORE_RESET = 3;
const int MIN_SIGNAL = 10;


// ============================================================
// SECTION 1 - WHICH ROOM IS THIS PIN?
// ============================================================
//
// The room name is looked up FROM the pin number, never typed in
// separately. That is deliberate: two settings that must agree are
// two settings that can disagree. Here there is only one.
//
// This mapping matches the main ABMDMS system exactly. Pin 2 is
// Room C because it held the very first sensor ever built.

const char* zoneName() {
  switch (TEST_PIN) {
    case 2:  return "ROOMC";
    case 3:  return "ROOMA";
    case 4:  return "ROOMB";
    case 5:  return "ROOMD";
    default: return "BADPIN";
  }
}

const char* zoneText() {
  switch (TEST_PIN) {
    case 2:  return "Room C";
    case 3:  return "Room A";
    case 4:  return "Room B";
    case 5:  return "Room D";
    default: return "unknown room";
  }
}

bool pinIsValid() {
  return TEST_PIN >= 2 && TEST_PIN <= 5;
}


// ============================================================
// SECTION 2 - MEMORY
// ============================================================

bool          motionActive = false;   // have we already announced motion?
unsigned long lowStartedAt = 0;       // when the sensor first went quiet

// --- SMS memory ---
SoftwareSerial sim(SIM_RX_PIN, SIM_TX_PIN);

bool          smsPending  = false;
unsigned long lastSmsAt   = 0;
bool          smsEverSent = false;    // millis() starts at 0, so without this
                                      // the very first alert would be blocked

bool simReady   = false;
int  smsFailRun = 0;

enum SmsState {
  SMS_IDLE,          // nothing to do
  SMS_WAIT_PROMPT,   // sent AT+CMGS="...", waiting for the ">" prompt
  SMS_WAIT_CONFIRM   // sent the message text, waiting for "+CMGS:"
};

SmsState      smsState        = SMS_IDLE;
unsigned long smsStateSince   = 0;
unsigned long smsLastFinished = 0;

const int SIM_BUF_SIZE = 64;
char simBuf[SIM_BUF_SIZE];
int  simBufLen = 0;


// ============================================================
// SECTION 3 - SETUP
// ============================================================

void setup() {
  Serial.begin(BAUD_RATE);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.println();
  Serial.println(F("============================================"));
  Serial.println(F("  ONE-PIN TEST"));
  Serial.print  ("  Watching Pin ");
  Serial.print  (TEST_PIN);
  Serial.print  ("  ->  ");
  Serial.print  (zoneName());
  Serial.print  ("  (");
  Serial.print  (zoneText());
  Serial.println(F(")"));
  Serial.println(F("============================================"));

  // Guard against a typo in TEST_PIN. Pins 0 and 1 are the USB
  // serial link, and anything above 5 is not one of our sensors -
  // either would look like a dead sensor and waste your time.
  if (!pinIsValid()) {
    Serial.println();
    Serial.print(F("STOP: TEST_PIN is set to "));
    Serial.print(TEST_PIN);
    Serial.println(F(", which is not a sensor pin."));
    Serial.println(F("Set it to 2, 3, 4 or 5 near the top of the sketch"));
    Serial.println(F("and upload again. Halted."));
    while (true) {
      // Blink fast forever so it is obvious something is wrong.
      digitalWrite(LED_PIN, HIGH);
      delay(120);
      digitalWrite(LED_PIN, LOW);
      delay(120);
    }
  }

  pinMode(TEST_PIN, INPUT);

  // Pin 5 is deliberately still supported here even though Room D has
  // been taken out of the rest of the system - this sketch is exactly
  // the tool you would use to find out whether that pin has started
  // working again.
  if (TEST_PIN == 5) {
    Serial.println();
    Serial.println(F("NOTE: Pin 5 / Room D is RETIRED from the main sketch."));
    Serial.println(F("      Two different sensors both failed on it, so suspect"));
    Serial.println(F("      the pin, the OUT wire or its rail tap - not the sensor."));
    Serial.println(F("      If it passes here, see SECTION 1 of pir_sms.ino for"));
    Serial.println(F("      how to put Room D back."));
    Serial.println();
  }

  Serial.println(F("The other sensors are ignored - leave them plugged in."));
  Serial.println();

  // Wake the GSM module first. It can find the network while the
  // PIR sensor warms up.
  simSetup();

  // The HC-SR501 gives false readings for the first few seconds
  // after power on. Wait it out. Stay away from the sensor.
  Serial.println();
  Serial.print(F("Warming up the sensor on Pin "));
  Serial.print(TEST_PIN);
  Serial.print(F(", please stay away from it ("));
  Serial.print(WARMUP_SECONDS);
  Serial.println(F(" seconds)..."));

  for (unsigned long i = WARMUP_SECONDS; i > 0; i--) {
    Serial.print(i);
    Serial.println(F("..."));
    delay(1000);
  }

  quietTest();

  Serial.println();
  Serial.println(F("System Ready"));
  Serial.print  ("Wave at the sensor on Pin ");
  Serial.print  (TEST_PIN);
  Serial.print  (" - it should print ");
  Serial.print  (zoneName());
  Serial.println(F("_MOTION_DETECTED"));
  Serial.println();
}


// ============================================================
// SECTION 4 - THE QUIET TEST
// ============================================================
//
// Straight after warm-up, watch the pin for ten seconds while you
// keep away from it. A correctly wired sensor sits perfectly still.
//
// A pin that flickers with nobody near it is almost always a
// FLOATING INPUT - the OUT wire is not actually making contact, so
// the pin picks up electrical noise and reports it as a person.
// This test tells you that in ten seconds instead of after an hour
// of confusing readings.

void quietTest() {

  Serial.println();
  Serial.print(F("Quiet test: watching Pin "));
  Serial.print(TEST_PIN);
  Serial.print(F(" for "));
  Serial.print(QUIET_TEST_MS / 1000);
  Serial.println(F(" seconds. Do not move near it."));

  unsigned long startedAt = millis();
  int  lastLevel = digitalRead(TEST_PIN);
  int  changes   = 0;
  long highCount = 0;
  long samples   = 0;

  while (millis() - startedAt < QUIET_TEST_MS) {
    int level = digitalRead(TEST_PIN);
    if (level != lastLevel) {
      changes++;
      lastLevel = level;
    }
    if (level == HIGH) {
      highCount++;
    }
    samples++;
    delay(10);
  }

  Serial.print(F("   Result: "));
  Serial.print(changes);
  Serial.print(F(" changes, HIGH for "));
  Serial.print((highCount * 100L) / (samples > 0 ? samples : 1));
  Serial.println(F("% of the time."));

  if (changes == 0 && highCount == 0) {
    Serial.println(F("   PASS - sensor is quiet and settled."));
  }
  else if (changes == 0 && highCount == samples) {
    Serial.println(F("   STUCK HIGH - the sensor sees motion the whole time."));
    Serial.println(F("   Either something is moving near it (a fan, a curtain,"));
    Serial.println(F("   sunlight), or its time-delay screw is turned right up."));
    Serial.println(F("   Turn that screw fully anti-clockwise and try again."));
  }
  else {
    Serial.println(F("   NOISY - it flickered with nobody near it."));
    Serial.println(F("   Most likely the OUT wire is not seated properly, so the"));
    Serial.println(F("   pin is floating and picking up noise."));
    Serial.print  ("   TEST: unplug the OUT wire and jumper Pin ");
    Serial.print  (TEST_PIN);
    Serial.println(F(" straight to GND."));
    Serial.println(F("   Goes quiet? The wiring is the problem, not the code."));
  }
}


// ============================================================
// SECTION 5 - MAIN LOOP
// ============================================================

void loop() {

  int sensorValue = digitalRead(TEST_PIN);

  // --- movement ---
  if (sensorValue == HIGH) {

    lowStartedAt = 0;   // cancel any "stop" countdown

    if (motionActive == false) {
      motionActive = true;
      digitalWrite(LED_PIN, HIGH);

      // The pin number is printed alongside the token on purpose:
      // if you forgot to change TEST_PIN before uploading, you see
      // it here instead of believing you tested a different sensor.
      Serial.print(zoneName());
      Serial.print(F("_MOTION_DETECTED"));
      Serial.print(F("        (Pin "));
      Serial.print(TEST_PIN);
      Serial.println(F(")"));

      queueSms();
    }
  }

  // --- no movement ---
  else {
    if (motionActive == true) {

      // The PIR output flickers as someone leaves, so wait a couple
      // of seconds before believing the room is empty.
      if (lowStartedAt == 0) {
        lowStartedAt = millis();
      }

      if (millis() - lowStartedAt >= STOP_CONFIRM_MS) {
        motionActive = false;
        lowStartedAt = 0;
        digitalWrite(LED_PIN, LOW);

        Serial.print(zoneName());
        Serial.print(F("_MOTION_STOPPED"));
        Serial.print(F("         (Pin "));
        Serial.print(TEST_PIN);
        Serial.println(F(")"));
        // No SMS on stop, on purpose. Only the START of motion texts.
      }
    }
  }

  smsTick();   // move any in-progress SMS forward one small step

  delay(50);
}


// ============================================================
// SECTION 6 - SMS: PUTTING A MESSAGE IN LINE
// ============================================================

void queueSms() {

  if (!SIM_ENABLED) {
    return;
  }

  // Cooldown: did we text recently? smsEverSent tells the difference
  // between "texted a long time ago" and "never texted at all".
  if (smsEverSent && (millis() - lastSmsAt < SMS_COOLDOWN_MS)) {
    Serial.print(F("SMS_SKIP:"));
    Serial.print(zoneName());
    Serial.println(F(":COOLDOWN"));
    return;
  }

  if (!simReady) {
    Serial.print(F("SMS_FAIL:"));
    Serial.print(zoneName());
    Serial.println(F(":NOMODULE"));
    return;
  }

  smsPending = true;
}


// ============================================================
// SECTION 7 - SMS: THE STEP-BY-STEP SENDER
// Called once per loop(). Does a tiny bit of work and returns
// straight away, so motion detection never pauses.
// ============================================================

void smsTick() {

  if (!SIM_ENABLED) {
    return;
  }

  switch (smsState) {

    case SMS_IDLE: {

      if (!simReady || !smsPending) {
        return;
      }

      // Check the cooldown AGAIN, here, right before sending.
      //
      // Checking it only in queueSms() is not enough. Sending a text
      // takes 5-10 seconds, and the PIR can easily see movement again
      // in that time. That second request is queued while the first
      // text is still in flight - at which point no text has finished
      // yet, so the cooldown legitimately does not apply and the
      // request is accepted. It then sits in smsPending and fires the
      // moment the first send completes, sending a SECOND text for
      // what should have been one alert.
      //
      // Re-checking here catches exactly that: by now the first text
      // HAS finished, so the cooldown is live and this stale request
      // is dropped.
      if (smsEverSent && (millis() - lastSmsAt < SMS_COOLDOWN_MS)) {
        smsPending = false;
        Serial.print(F("SMS_SKIP:"));
        Serial.print(zoneName());
        Serial.println(F(":COOLDOWN"));
        return;
      }

      // Give the module its rest before starting another message.
      if (smsLastFinished != 0 && (millis() - smsLastFinished < SMS_MIN_GAP_MS)) {
        return;   // the request stays pending; we come back to it
      }

      smsPending = false;

      simBufClear();
      sim.print(F("AT+CMGS=\""));
      sim.print(SMS_RECIPIENT);
      sim.print(F("\"\r"));

      smsState      = SMS_WAIT_PROMPT;
      smsStateSince = millis();
      break;
    }

    case SMS_WAIT_PROMPT: {

      simDrain();

      if (simSaw(">")) {
        sim.print(F("ABMDMS TEST: Motion detected in "));
        sim.print(zoneText());
        sim.print(F(" ("));
        sim.print(zoneName());
        sim.print(F(", Pin "));
        sim.print(TEST_PIN);
        sim.print(F("). Uptime "));
        sim.print(millis() / 60000UL);
        sim.print(F(" min."));
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


// Ends the current send, prints the result, and starts the cooldown
// either way (a failing module must not be hammered with a retry on
// every single movement).
void smsFinish(bool ok, const char* reason) {

  // If we are giving up part-way through, the module may still be
  // sitting at its ">" prompt waiting for more text. ESC abandons
  // that half-typed message. Without this the module stays in
  // text-entry mode and swallows the NEXT AT+CMGS, so one failure
  // turns into an endless run of them.
  if (!ok) {
    sim.write((char) 27);   // ESC = throw this message away
    sim.print(F("\r"));
  }

  lastSmsAt   = millis();
  smsEverSent = true;

  if (ok) {
    Serial.print(F("SMS_SENT:"));
    Serial.println(zoneName());
  } else {
    Serial.print(F("SMS_FAIL:"));
    Serial.print(zoneName());
    Serial.print(F(":"));
    Serial.println(reason);
  }

  smsFailRun = ok ? 0 : (smsFailRun + 1);

  smsState        = SMS_IDLE;
  smsLastFinished = millis();
  simBufClear();

  if (smsFailRun >= SMS_MAX_FAILS_BEFORE_RESET) {
    smsFailRun = 0;
    simReset();
  }
}


// ============================================================
// SECTION 8 - SMS: TALKING TO THE MODULE
// ============================================================

void simBufClear() {
  simBufLen = 0;
  simBuf[0] = '\0';
}


// Copy whatever the module has said into the buffer.
// Never waits - if there is nothing to read it returns at once.
void simDrain() {
  while (sim.available()) {
    char c = (char) sim.read();

    if (c == '\0') {
      continue;
    }

    // Buffer full? Throw away the oldest half and keep going.
    if (simBufLen >= SIM_BUF_SIZE - 1) {
      int keep = SIM_BUF_SIZE / 2;
      memmove(simBuf, simBuf + (simBufLen - keep), keep);
      simBufLen = keep;
    }

    simBuf[simBufLen++] = c;
    simBuf[simBufLen]   = '\0';
  }
}


// Pull the number out of a "+CSQ: 18,0" reply. -1 if not there.
int parseCsq() {
  char* at = strstr(simBuf, "+CSQ:");
  if (at == NULL) {
    return -1;
  }
  return atoi(at + 5);   // atoi skips the space, stops at the comma
}


// Did the module say this anywhere in what we collected? We search
// for a PIECE of text, not an exact match, on purpose: SoftwareSerial
// sometimes drops a character, and an exact match would then fail a
// send that actually worked.
bool simSaw(const char* token) {
  return strstr(simBuf, token) != NULL;
}


// Send one AT command and wait for the expected reply. This one IS
// allowed to wait, because it only runs in setup().
bool simCommand(const char* command, const char* expect, unsigned long timeoutMs) {

  simBufClear();
  sim.print(command);
  sim.print(F("\r"));

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
void simSetup() {

  if (!SIM_ENABLED) {
    Serial.println(F("SIM_FAIL:DISABLED  (SIM_ENABLED is false - PIR test only)"));
    return;
  }

  pinMode(SIM_RST_PIN, OUTPUT);
  digitalWrite(SIM_RST_PIN, HIGH);   // RST is active LOW - keep it released

  sim.begin(SIM_BAUD_RATE);
  Serial.println(F("Starting SIM800L, please wait..."));
  delay(3000);                       // the module needs a moment after power on

  // 1. Is it alive?
  if (!simCommand("AT", "OK", 5000)) {
    Serial.println(F("SIM_FAIL:NOREPLY"));
    Serial.println(F("   Check: external power, common GND, TX/RX not swapped."));
    simReady = false;
    return;
  }

  // 2. Turn OFF command echo. By default the module repeats every
  //    command back before answering it, and that echo lands in the
  //    same buffer we search for replies - so "AT+CSQ" itself looks
  //    like a "+CSQ" answer and we read the question, not the reading.
  simCommand("ATE0", "OK", 3000);

  // 3. Ask for real error numbers instead of a bare "ERROR".
  simCommand("AT+CMEE=2", "OK", 3000);

  // 4. Is the SIM card in and unlocked?
  if (!simCommand("AT+CPIN?", "READY", 5000)) {
    Serial.println(F("SIM_FAIL:NOSIM"));
    Serial.println(F("   Check: SIM inserted properly, PIN lock turned OFF."));
    simReady = false;
    return;
  }

  // 5. How strong is the signal? 0-31, bigger is better.
  //    99 means no signal at all. Under 10 is too weak to be reliable.
  simCommand("AT+CSQ", "+CSQ", 5000);
  int signal = parseCsq();
  Serial.print(F("   Signal: "));
  if (signal < 0) {
    Serial.println(F("could not read"));
  } else if (signal == 99) {
    Serial.println(F("99 - NO SIGNAL (check the antenna)"));
  } else {
    Serial.print(signal);
    Serial.println(signal < MIN_SIGNAL ? F(" - WEAK, sending may fail") : F(" - ok"));
  }

  // 6. Did it join the network? ",1" = home, ",5" = roaming
  bool registered = simCommand("AT+CREG?", ",1", 10000);
  if (!registered) {
    registered = simCommand("AT+CREG?", ",5", 10000);
  }
  if (!registered) {
    Serial.println(F("SIM_FAIL:NONETWORK"));
    Serial.println(F("   Check: antenna, load/credit, and 2G coverage here."));
    simReady = false;
    return;
  }

  // 7. Plain text messages, not the binary PDU format.
  if (!simCommand("AT+CMGF=1", "OK", 5000)) {
    Serial.println(F("SIM_FAIL:TEXTMODE"));
    simReady = false;
    return;
  }

  simReady = true;
  Serial.println(F("SIM_READY"));
  Serial.print(F("   Alerts will be sent to: "));
  Serial.println(SMS_RECIPIENT);
}


// Hard-reset the module by pulling RST low for a moment.
// Used when several sends fail in a row.
void simReset() {
  Serial.println(F("SIM_RESET"));

  digitalWrite(SIM_RST_PIN, LOW);
  delay(150);
  digitalWrite(SIM_RST_PIN, HIGH);

  // The module is unusable while it restarts. A reset also forgets
  // ATE0 and text mode, so both have to be set again.
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
