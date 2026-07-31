/*
  ============================================================
  SIM800L SELF TEST — TEST C
  File  : sim800l_selftest.ino
  Board : Arduino Uno
  Part  : ABMDMS / sms_module_test  (bench test only)
  ============================================================

  WHAT THIS IS
  ------------
  The same checks you did by hand in sim800l_passthrough.ino, but run
  automatically with a clear PASS / FAIL for each step. Use it to prove
  the module works, to re-check it quickly before a demo, and to show
  someone the result without them having to know AT commands.

  Nothing here touches the PIR sensors, the database, or the dashboard.
  This is a bench test of the GSM module on its own.

  WIRING  (SIM800L V2.2 by UNV — the 5V board)
  ------
  SIM800L TXD  ->  Arduino Pin 10          (direct)
  SIM800L RXD  ->  Arduino Pin 11          (direct - V2.2 is 5V-logic, no divider)
  SIM800L RST  ->  Arduino Pin 12
  SIM800L 5Vin ->  power bank 5V           (NEVER the Arduino 5V pin or DC jack)
  SIM800L GND  ->  power bank GND AND Arduino GND   (all grounds common)
  SIM800L VDD  ->  leave UNCONNECTED       (2.8V reference output, not a power pin)
  1000uF capacitor ACROSS the module's 5Vin / GND. Antenna on BEFORE power.

  HOW TO USE IT
  -------------
  1. Put YOUR OWN number in TEST_RECIPIENT below (international: +639171234567).
  2. Leave SEND_REAL_SMS as false for a dry run (checks 1-5 only, spends nothing).
     Set it to true when you want it to actually send the text.
  3. Upload, then open Tools > Serial Monitor at 9600.
  4. The test runs once at boot. Type 't' and press Enter to run it again.

  WHAT EACH STEP MEANS
  --------------------
  1 AT        - the module is powered and the two data wires are right
  2 AT+CPIN?  - the SIM is seated and its PIN lock is off
  3 AT+CSQ    - there is usable 2G signal (above 10; 99 means none at all)
  4 AT+CREG?  - the network actually accepted this SIM (0,1 home / 0,5 roaming)
  5 AT+CMGF=1 - plain-text SMS mode, the mode the real sketch uses
  6 AT+CMGS   - a real message goes out

  A failure at step 1 is wiring. Steps 3-4 are coverage. Step 6 is usually
  load/credit, a wrongly formatted number, or the module browning out.
  ============================================================
*/

#include <SoftwareSerial.h>


// ============================================================
// SECTION 1 - SETTINGS
// Change these two lines. Everything else can stay as it is.
// ============================================================

// Your phone number, international format, no spaces. THIS IS THE ONE TO EDIT.
const char* TEST_RECIPIENT = "+639169751409";

// false = run checks 1-5 only, send nothing (free, safe to repeat)
// true  = also send one real text message (costs one SMS)
const bool SEND_REAL_SMS = false;

// The message that gets sent. Keep it plain ASCII - no emoji, no accents.
const char* TEST_MESSAGE = "ABMDMS SIM800L bench test - if you can read this, the module can send 2G SMS.";

// Signal strength below this counts as a fail. 10 is the usual usable floor.
const int MIN_SIGNAL = 10;


// ------------------------------------------------------------
// PINS - these match arduino/motion_sensor/motion_sensor.ino on purpose
// ------------------------------------------------------------

const int SIM_RX_PIN  = 10;   // Arduino Pin 10 <- SIM800L TXD
const int SIM_TX_PIN  = 11;   // Arduino Pin 11 -> SIM800L RXD
const int SIM_RST_PIN = 12;   // Arduino Pin 12 -> SIM800L RST (active LOW)

const unsigned long PC_BAUD_RATE  = 9600;
const unsigned long SIM_BAUD_RATE = 9600;

// How long to wait for each kind of reply. Sending is slow - the module has
// to hand the message to the tower - so it gets much longer than the rest.
const unsigned long REPLY_TIMEOUT_MS = 3000;
const unsigned long SEND_TIMEOUT_MS  = 60000;


// ============================================================
// SECTION 2 - STATE
// ============================================================

SoftwareSerial sim(SIM_RX_PIN, SIM_TX_PIN);

String reply = "";       // the module's last answer, kept for printing
int    failures = 0;     // how many steps failed in this run


// ============================================================
// SECTION 3 - SETUP
// ============================================================

void setup() {
  Serial.begin(PC_BAUD_RATE);

  pinMode(SIM_RST_PIN, OUTPUT);
  digitalWrite(SIM_RST_PIN, HIGH);   // RST is active LOW - keep it released

  sim.begin(SIM_BAUD_RATE);

  Serial.println();
  Serial.println(F("Waiting 10s for the SIM800L to join the network..."));
  Serial.println(F("(status LED should settle to one blink every ~3 seconds)"));
  delay(10000);

  runSelfTest();
}


// ============================================================
// SECTION 4 - LOOP
// Only job: let you press 't' to run the test again.
// ============================================================

void loop() {
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 't' || c == 'T') {
      runSelfTest();
    }
  }
}


// ============================================================
// SECTION 5 - THE TEST ITSELF
// ============================================================

void runSelfTest() {
  failures = 0;

  Serial.println();
  Serial.println(F("=== SIM800L SELF TEST ==="));

  // ---- 1. Is it alive? ------------------------------------
  Serial.print(F("[1/6] Module responds (AT) ............ "));
  if (askAndExpect("AT", "OK", REPLY_TIMEOUT_MS)) {
    pass("");
  } else {
    fail(F("no reply - check external power, common ground, TX/RX not swapped"));
    verdict();                 // nothing else can pass, so stop here
    return;
  }

  // Turn the echo off so the answers we parse are clean.
  askAndExpect("ATE0", "OK", REPLY_TIMEOUT_MS);

  // ---- 2. Is the SIM ready? -------------------------------
  Serial.print(F("[2/6] SIM ready (AT+CPIN?) ............ "));
  if (askAndExpect("AT+CPIN?", "+CPIN: READY", REPLY_TIMEOUT_MS)) {
    pass("");
  } else {
    fail(F("SIM not seated, or its PIN lock is still on"));
  }

  // ---- 3. Is there signal? --------------------------------
  Serial.print(F("[3/6] Signal quality (AT+CSQ) ......... "));
  ask("AT+CSQ", REPLY_TIMEOUT_MS);
  int rssi = parseCsq(reply);
  if (rssi >= MIN_SIGNAL && rssi != 99) {
    pass(String("rssi=") + rssi + (rssi >= 20 ? " strong" : " good"));
  } else if (rssi == 99 || rssi < 0) {
    fail(F("no signal at all (99) - antenna not attached, or no 2G coverage here"));
  } else {
    pass(String("rssi=") + rssi + " WEAK - may fail under load");
  }

  // ---- 4. Did the network accept us? ----------------------
  Serial.print(F("[4/6] Network registered (AT+CREG?) ... "));
  ask("AT+CREG?", REPLY_TIMEOUT_MS);
  if (reply.indexOf("0,1") >= 0) {
    pass(F("0,1 home network"));
  } else if (reply.indexOf("0,5") >= 0) {
    pass(F("0,5 roaming"));
  } else if (reply.indexOf("0,2") >= 0) {
    fail(F("0,2 still searching - wait 30s and retry, or 2G is off in this area"));
  } else if (reply.indexOf("0,3") >= 0) {
    fail(F("0,3 registration DENIED - SIM barred, expired, or no load"));
  } else {
    fail(F("not registered - check the SIM in a 2G-forced phone in this room"));
  }

  // ---- 5. Plain-text mode ---------------------------------
  Serial.print(F("[5/6] Text mode (AT+CMGF=1) ........... "));
  if (askAndExpect("AT+CMGF=1", "OK", REPLY_TIMEOUT_MS)) {
    pass("");
  } else {
    fail(F("module refused text mode - firmware or a bad connection"));
  }

  // ---- 6. Actually send one ------------------------------
  Serial.print(F("[6/6] Send SMS to "));
  Serial.print(TEST_RECIPIENT);
  Serial.print(F(" ....... "));

  if (!SEND_REAL_SMS) {
    Serial.println(F("SKIPPED (SEND_REAL_SMS is false)"));
  } else if (failures > 0) {
    Serial.println(F("SKIPPED (fix the failures above first)"));
  } else {
    sendTestSms();
  }

  verdict();
}


// The send is its own function because it is the only multi-part exchange:
// the module answers with '>' first, and only then do we hand it the text.
void sendTestSms() {
  sim.print("AT+CMGS=\"");
  sim.print(TEST_RECIPIENT);
  sim.println("\"");

  if (!waitFor(">", REPLY_TIMEOUT_MS)) {
    fail(F("no '>' prompt - number must be international format, e.g. +639171234567"));
    return;
  }

  sim.print(TEST_MESSAGE);
  sim.write((char)26);            // Ctrl+Z = "message finished, send it"

  if (waitFor("+CMGS:", SEND_TIMEOUT_MS)) {
    pass(trimReply(reply));
    Serial.println(F("      -> check your phone now."));
  } else if (reply.indexOf("ERROR") >= 0) {
    fail(F("module returned ERROR - usually no load/credit on the SIM"));
  } else {
    fail(F("timed out - weak signal, or the module browned out (check the 1000uF cap)"));
  }
}


void verdict() {
  Serial.println();
  if (failures == 0 && SEND_REAL_SMS) {
    Serial.println(F("RESULT: ALL PASS - the module can send 2G SMS."));
    Serial.println(F("Next: SIM800L_SMS_CHECKLIST.md Phase 4."));
  } else if (failures == 0) {
    Serial.println(F("RESULT: CHECKS PASS - set SEND_REAL_SMS to true to send a real text."));
  } else {
    Serial.print(F("RESULT: "));
    Serial.print(failures);
    Serial.println(F(" FAILED - see AT_COMMANDS.md for the fix table."));
  }
  Serial.println(F("Type 't' to run again."));
  Serial.println(F("========================="));
}


// ============================================================
// SECTION 6 - TALKING TO THE MODULE
// ============================================================

// Send one command and collect whatever comes back into 'reply'.
void ask(const char* command, unsigned long timeoutMs) {
  while (sim.available()) sim.read();     // throw away anything left over
  sim.println(command);
  collect(timeoutMs);
}


// Send one command and say whether the answer contained what we wanted.
bool askAndExpect(const char* command, const char* wanted, unsigned long timeoutMs) {
  ask(command, timeoutMs);
  return reply.indexOf(wanted) >= 0;
}


// Read until 'wanted' shows up, or the time runs out. Used for the two-part send.
bool waitFor(const char* wanted, unsigned long timeoutMs) {
  reply = "";
  unsigned long started = millis();
  while (millis() - started < timeoutMs) {
    while (sim.available()) {
      reply += (char)sim.read();
      if (reply.indexOf(wanted) >= 0) return true;
      if (reply.indexOf("ERROR")  >= 0) return false;
    }
  }
  return false;
}


// Gather the module's answer. Stops early once OK or ERROR arrives, so a
// passing step does not sit through the whole timeout.
void collect(unsigned long timeoutMs) {
  reply = "";
  unsigned long started = millis();
  while (millis() - started < timeoutMs) {
    while (sim.available()) {
      reply += (char)sim.read();
    }
    if (reply.indexOf("OK") >= 0 || reply.indexOf("ERROR") >= 0) {
      delay(20);                          // let the last few bytes land
      while (sim.available()) reply += (char)sim.read();
      return;
    }
  }
}


// ============================================================
// SECTION 7 - SMALL HELPERS
// ============================================================

// Pull the first number out of "+CSQ: 17,0". Returns -1 if it is not there.
int parseCsq(String s) {
  int at = s.indexOf("+CSQ:");
  if (at < 0) return -1;
  return s.substring(at + 5).toInt();
}


// Squash a multi-line module reply onto one line so it prints tidily.
String trimReply(String s) {
  s.replace("\r", " ");
  s.replace("\n", " ");
  s.trim();
  return s;
}


void pass(String note) {
  Serial.print(F("PASS"));
  if (note.length()) {
    Serial.print(F("  ("));
    Serial.print(note);
    Serial.print(F(")"));
  }
  Serial.println();
}


void fail(const __FlashStringHelper* why) {
  failures++;
  Serial.println(F("FAIL"));
  Serial.print(F("      -> "));
  Serial.println(why);
}
