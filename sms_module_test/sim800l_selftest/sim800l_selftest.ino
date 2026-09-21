/*
  ============================================================
  SIM800L SELF TEST — ESP32 VERSION
  File  : sim800l_selftest.ino
  Board : ESP32 Dev Module
  Part  : ABMDMS / sms_module_test
  ============================================================

  WHAT THIS DOES:
  ---------------
  Runs automatic hardware and network diagnostics on your SIM800L module
  using ESP32 Hardware UART2 (GPIO 16 / GPIO 17).
  Prints clear PASS / FAIL for each step:
    1. AT response (wiring & power)
    2. SIM Card detection & PIN lock status
    3. 2G Signal quality (RSSI)
    4. Cellular network registration (Home/Roaming)
    5. SMS text mode initialization
    6. (Optional) Real SMS transmission to your phone

  WIRING (ESP32):
  ---------------
  SIM800L TXD  ->  ESP32 GPIO 16 (RX2)
  SIM800L RXD  ->  ESP32 GPIO 17 (TX2)
  SIM800L RST  ->  ESP32 GPIO 4
  SIM800L 5Vin ->  HW-131 5V Rail (with 12V wall adapter)
  SIM800L GND  ->  HW-131 GND AND ESP32 GND (Common Ground)
  1000uF capacitor in parallel across 5Vin and GND (stripe to GND).
  Antenna screwed on firmly.

  SERIAL MONITOR SETTINGS:
  ------------------------
  - Baud rate: 115200
  - Line ending: Both NL & CR
*/

#include <Arduino.h>

// ============================================================
// SECTION 1 - SETTINGS
// ============================================================

// Your phone number in international format (+639XXXXXXXXX)
const char* TEST_RECIPIENT = "+639169751409";

// false = run checks 1-5 only, send nothing (free, safe to repeat)
// true  = also send one real text message (costs one SMS)
const bool SEND_REAL_SMS = false;

// The message that gets sent if SEND_REAL_SMS is true
const char* TEST_MESSAGE = "ABMDMS SIM800L ESP32 test - module is working perfectly!";

const int MIN_SIGNAL = 10; // Signal floor

// ESP32 Hardware UART2 Pins
const int SIM_RX_PIN  = 16;  // ESP32 GPIO 16 (RX2) <- SIM800L TXD
const int SIM_TX_PIN  = 17;  // ESP32 GPIO 17 (TX2) -> SIM800L RXD
const int SIM_RST_PIN = 4;   // ESP32 GPIO 4        -> SIM800L RST

const unsigned long PC_BAUD_RATE  = 115200;
const unsigned long SIM_BAUD_RATE = 9600;

const unsigned long REPLY_TIMEOUT_MS = 4000;
const unsigned long SEND_TIMEOUT_MS  = 45000;

// ============================================================
// SECTION 2 - STATE & FORWARD DECLARATIONS
// ============================================================

HardwareSerial sim(2); // Hardware UART2

String reply = "";
int    failures = 0;

void runSelfTest();
void sendTestSms();
void verdict();
void ask(const char* command, unsigned long timeoutMs);
bool askAndExpect(const char* command, const char* wanted, unsigned long timeoutMs);
bool waitFor(const char* wanted, unsigned long timeoutMs);
void collect(unsigned long timeoutMs);
int  parseCsq(String s);
String trimReply(String s);
void pass(String note);
void fail(const char* why);

// ============================================================
// SECTION 3 - SETUP
// ============================================================

void setup() {
  Serial.begin(PC_BAUD_RATE);
  delay(1000);

  pinMode(SIM_RST_PIN, OUTPUT);
  digitalWrite(SIM_RST_PIN, HIGH); // Active LOW

  // Initialize Hardware UART2 on ESP32
  sim.begin(SIM_BAUD_RATE, SERIAL_8N1, SIM_RX_PIN, SIM_TX_PIN);

  Serial.println(F("\n============================================"));
  Serial.println(F("ABMDMS - SIM800L ESP32 AUTOMATED SELF-TEST"));
  Serial.println(F("============================================"));
  Serial.println(F("Waiting 5s for the SIM800L power-on & network search..."));
  Serial.println(F("(Watch the SIM800L LED: it should blink once every ~3 seconds)\n"));
  
  delay(5000);

  runSelfTest();
}

// ============================================================
// SECTION 4 - LOOP
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

  Serial.println(F("\n----------------- DIAGNOSTIC RUN -----------------"));

  // ---- 1. Check basic AT communication ----
  Serial.print(F("[1/6] Module responds (AT) ............ "));
  
  // Try sending AT up to 3 times to sync autobaud
  bool alive = false;
  for (int attempt = 0; attempt < 3; attempt++) {
    if (askAndExpect("AT", "OK", 1500)) {
      alive = true;
      break;
    }
    delay(300);
  }

  if (alive) {
    pass("");
  } else {
    fail("NO RESPONSE to AT!\n      * Check: 12V adapter plugged into HW-131 breadboard power\n      * Check: Common GND wire between ESP32 and HW-131\n      * Check: Try swapping GPIO 16 & GPIO 17 jumper wires");
    verdict();
    return;
  }

  // Turn off echo and enable detailed error numbers
  askAndExpect("ATE0", "OK", REPLY_TIMEOUT_MS);
  askAndExpect("AT+CMEE=2", "OK", REPLY_TIMEOUT_MS);

  // ---- 2. Is SIM card detected & unlocked? ----
  Serial.print(F("[2/6] SIM card status (AT+CPIN?) ...... "));
  if (askAndExpect("AT+CPIN?", "+CPIN: READY", REPLY_TIMEOUT_MS)) {
    pass("SIM Card Detected & Ready");
  } else {
    fail("SIM NOT DETECTED or PIN locked (Check if SIM is inserted firmly)");
  }

  // ---- 3. Signal Quality Check ----
  Serial.print(F("[3/6] Signal quality (AT+CSQ) ......... "));
  ask("AT+CSQ", REPLY_TIMEOUT_MS);
  int rssi = parseCsq(reply);
  if (rssi >= MIN_SIGNAL && rssi != 99) {
    pass(String("RSSI = ") + rssi + (rssi >= 20 ? " (Strong signal)" : " (Good signal)"));
  } else if (rssi == 99 || rssi < 0) {
    fail("NO SIGNAL (RSSI 99) - Screw on the antenna firmly & check 2G coverage");
  } else {
    pass(String("RSSI = ") + rssi + " (WEAK signal - may fail under load)");
  }

  // ---- 4. Network Registration Check ----
  Serial.print(F("[4/6] Network registered (AT+CREG?) ... "));
  ask("AT+CREG?", REPLY_TIMEOUT_MS);
  if (reply.indexOf("0,1") >= 0 || reply.indexOf("1,1") >= 0) {
    pass("Registered (Home Network)");
  } else if (reply.indexOf("0,5") >= 0 || reply.indexOf("1,5") >= 0) {
    pass("Registered (Roaming Network)");
  } else if (reply.indexOf("0,2") >= 0) {
    fail("Still searching for cell tower (Wait 30s and press 't' to test again)");
  } else if (reply.indexOf("0,3") >= 0) {
    fail("Registration DENIED by carrier (SIM expired, barred, or zero load)");
  } else {
    fail(String("Not registered on cell network (Reply: " + trimReply(reply) + ")").c_str());
  }

  // ---- 5. Plain-Text SMS Mode ----
  Serial.print(F("[5/6] SMS text mode (AT+CMGF=1) ....... "));
  if (askAndExpect("AT+CMGF=1", "OK", REPLY_TIMEOUT_MS)) {
    pass("Text Mode Enabled");
  } else {
    fail("Module refused text mode");
  }

  // ---- 6. Real SMS Test ----
  Serial.print(F("[6/6] Send SMS to "));
  Serial.print(TEST_RECIPIENT);
  Serial.print(F(" ....... "));

  if (!SEND_REAL_SMS) {
    Serial.println(F("SKIPPED (Set SEND_REAL_SMS = true in sketch to test actual SMS)"));
  } else if (failures > 0) {
    Serial.println(F("SKIPPED (Resolve failures above first)"));
  } else {
    sendTestSms();
  }

  verdict();
}

void sendTestSms() {
  sim.print("AT+CMGS=\"");
  sim.print(TEST_RECIPIENT);
  sim.println("\"");

  if (!waitFor(">", REPLY_TIMEOUT_MS)) {
    fail("No '>' prompt received from module");
    return;
  }

  sim.print(TEST_MESSAGE);
  sim.write((char)26); // Ctrl+Z

  if (waitFor("+CMGS:", SEND_TIMEOUT_MS)) {
    pass(trimReply(reply));
    Serial.println(F("      -> Check your mobile phone for the test message!"));
  } else if (reply.indexOf("ERROR") >= 0) {
    fail("Module returned ERROR (Check SIM prepaid balance / load)");
  } else {
    fail("Timed out waiting for network acceptance (Check 1000uF capacitor)");
  }
}

void verdict() {
  Serial.println(F("--------------------------------------------------"));
  if (failures == 0 && SEND_REAL_SMS) {
    Serial.println(F("RESULT: ALL 6 TESTS PASSED! SIM800L is 100% operational."));
  } else if (failures == 0) {
    Serial.println(F("RESULT: HARDWARE CHECKS PASSED! Set SEND_REAL_SMS = true to test sending."));
  } else {
    Serial.printf("RESULT: %d STEP(S) FAILED. Follow the suggestions above.\n", failures);
  }
  Serial.println(F("Type 't' in Serial Monitor and press Enter to test again."));
  Serial.println(F("==================================================\n"));
}

// ============================================================
// SECTION 6 - SERIAL COMMUNICATION HELPERS
// ============================================================

void ask(const char* command, unsigned long timeoutMs) {
  while (sim.available()) sim.read();
  sim.println(command);
  collect(timeoutMs);
}

bool askAndExpect(const char* command, const char* wanted, unsigned long timeoutMs) {
  ask(command, timeoutMs);
  return reply.indexOf(wanted) >= 0;
}

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

void collect(unsigned long timeoutMs) {
  reply = "";
  unsigned long started = millis();
  while (millis() - started < timeoutMs) {
    while (sim.available()) {
      reply += (char)sim.read();
    }
    if (reply.indexOf("OK") >= 0 || reply.indexOf("ERROR") >= 0) {
      delay(20);
      while (sim.available()) reply += (char)sim.read();
      return;
    }
  }
}

int parseCsq(String s) {
  int at = s.indexOf("+CSQ:");
  if (at < 0) return -1;
  return s.substring(at + 5).toInt();
}

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

void fail(const char* why) {
  failures++;
  Serial.println(F("FAIL"));
  Serial.print(F("      -> "));
  Serial.println(why);
}
