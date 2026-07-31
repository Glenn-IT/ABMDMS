/*
  ============================================================
  SIM800L PASSTHROUGH — TEST B
  File  : sim800l_passthrough.ino
  Board : Arduino Uno
  Part  : ABMDMS / sms_module_test  (bench test only)
  ============================================================

  WHAT THIS IS
  ------------
  A wire, not a program. Everything you type in the Serial Monitor is
  handed straight to the SIM800L, and everything the SIM800L says comes
  straight back. It lets you talk to the module by hand so you can find
  out whether it can send a text BEFORE any real system code is involved.

  Nothing here touches the PIR sensors, the database, or the dashboard.

  WIRING  (SIM800L V2.2 by UNV — the 5V board)
  ------
  SIM800L TXD  ->  Arduino Pin 10          (direct)
  SIM800L RXD  ->  Arduino Pin 11          (direct - V2.2 is 5V-logic, no divider)
  SIM800L RST  ->  Arduino Pin 12          (optional)
  SIM800L 5Vin ->  power bank 5V           (NEVER the Arduino 5V pin or DC jack)
  SIM800L GND  ->  power bank GND AND Arduino GND   (all grounds common)
  SIM800L VDD  ->  leave UNCONNECTED       (2.8V reference output, not a power pin)
  1000uF capacitor ACROSS the module's 5Vin / GND (parallel, stripe leg on GND).
  Antenna screwed on BEFORE power is applied.

  HOW TO USE IT
  -------------
  1. Upload this sketch.
  2. Tools > Serial Monitor, 9600 baud.
  3. Set the line-ending dropdown (bottom right) to "Both NL & CR".
     If this is wrong the module ignores you and you will think it is dead.
  4. Type the commands from ../AT_COMMANDS.md, one at a time.

  The one that matters:
     AT+CMGF=1                     -> OK
     AT+CMGS="+639171234567"       -> >
     type your message
     press Ctrl+Z (send it as a character)  -> +CMGS: 42
  ...and a real text arrives on your phone.

  Ctrl+Z NOTE: the Arduino IDE Serial Monitor cannot send Ctrl+Z. Type the
  three letters  END  on its own line instead and this sketch converts it to
  the Ctrl+Z character (26) for you. See sendCtrlZ() below.
  ============================================================
*/

#include <SoftwareSerial.h>


// ============================================================
// SECTION 1 - SETTINGS
// These match arduino/motion_sensor/motion_sensor.ino on purpose,
// so a passing test here means the real sketch is wired the same.
// ============================================================

const int SIM_RX_PIN  = 10;   // Arduino Pin 10 <- SIM800L TXD
const int SIM_TX_PIN  = 11;   // Arduino Pin 11 -> SIM800L RXD
const int SIM_RST_PIN = 12;   // Arduino Pin 12 -> SIM800L RST (active LOW)

const unsigned long PC_BAUD_RATE  = 9600;   // Serial Monitor speed
const unsigned long SIM_BAUD_RATE = 9600;   // SIM800L default speed


// ============================================================
// SECTION 2 - THE SOFTWARE SERIAL PORT
// The Uno only has one real serial port and the USB cable is using it,
// so we make a second one in software on pins 10 and 11.
// ============================================================

SoftwareSerial sim(SIM_RX_PIN, SIM_TX_PIN);

String typed = "";        // the line you are currently typing
bool   lineJustEnded = false;   // used to swallow the second half of a "\r\n" pair


// ============================================================
// SECTION 3 - SETUP
// ============================================================

void setup() {
  Serial.begin(PC_BAUD_RATE);

  // RST is active LOW. Keep it HIGH so the module is NOT held in reset.
  pinMode(SIM_RST_PIN, OUTPUT);
  digitalWrite(SIM_RST_PIN, HIGH);

  sim.begin(SIM_BAUD_RATE);

  delay(1000);   // a plain delay is fine here - this sketch has nothing else to do

  Serial.println(F("============================================"));
  Serial.println(F("SIM800L PASSTHROUGH - type AT commands below"));
  Serial.println(F("============================================"));
  Serial.println(F("Serial Monitor line ending MUST be 'Both NL & CR'."));
  Serial.println(F(""));
  Serial.println(F("Try, one at a time:"));
  Serial.println(F("  AT+CMEE=2       -> OK              (real error text, not just 'ERROR')"));
  Serial.println(F("  AT              -> OK              (module alive)"));
  Serial.println(F("  AT+CPIN?        -> +CPIN: READY    (SIM in, no PIN lock)"));
  Serial.println(F("  AT+CSQ          -> +CSQ: 17,0      (want above 10, 99 = none)"));
  Serial.println(F("  AT+CREG?        -> +CREG: 0,1      (0,1 home / 0,5 roaming)"));
  Serial.println(F("  AT+CSCA?        -> +CSCA: \"+639...\"  (service centre - EMPTY means sends fail)"));
  Serial.println(F("  AT+CMGF=1       -> OK              (plain text mode)"));
  Serial.println(F("  AT+CMGS=\"+639171234567\"  -> >     (then type the message)"));
  Serial.println(F("  END             -> sends Ctrl+Z, the message goes out"));
  Serial.println(F(""));
  Serial.println(F("Stuck at the '>' prompt? Everything you type there is message text."));
  Serial.println(F("  END             -> send the message"));
  Serial.println(F("  CANCEL          -> throw it away and get back to AT commands"));
  Serial.println(F("  RESET           -> hardware-reset the module"));
  Serial.println(F("--------------------------------------------"));
}


// ============================================================
// SECTION 4 - THE LOOP
// Copy bytes in both directions, and watch for our two helper words.
//
// Your typing is collected a LINE at a time and only handed to the module
// when you press Enter. That matters: if the characters went straight
// through as you typed them, the word END would land inside the message
// body before we got the chance to turn it into Ctrl+Z, and you would send
// "SupEND" instead of "Sup".
// ============================================================

void loop() {
  // ---- SIM800L -> Serial Monitor -------------------------
  while (sim.available()) {
    Serial.write(sim.read());
  }

  // ---- Serial Monitor -> SIM800L -------------------------
  while (Serial.available()) {
    char c = Serial.read();

    if (c == '\r' || c == '\n') {
      // "Both NL & CR" sends TWO characters per Enter. Act on the first,
      // swallow the second, so one Enter is never read as two lines.
      if (lineJustEnded) { lineJustEnded = false; continue; }
      sendLine();
      lineJustEnded = true;
    } else {
      lineJustEnded = false;
      if (typed.length() < 200) typed += c;   // don't let a stray paste run away
    }
  }
}


// One finished line. Either it is one of our two helper words, or it goes
// to the module as-is with a proper carriage return on the end.
void sendLine() {
  typed.trim();

  if (typed.equalsIgnoreCase("END")) {
    sendCtrlZ();
  } else if (typed.equalsIgnoreCase("CANCEL")) {
    sendEscape();
  } else if (typed.equalsIgnoreCase("RESET")) {
    resetModule();
  } else {
    sim.print(typed);
    sim.print("\r\n");
  }

  typed = "";
}


// ============================================================
// SECTION 5 - HELPERS
// ============================================================

// Ctrl+Z (ASCII 26) is what tells the SIM800L "the message is finished,
// send it now". The IDE's Serial Monitor cannot type it, so we send it here.
void sendCtrlZ() {
  sim.write((char)26);
  Serial.println();
  Serial.println(F("[sent Ctrl+Z - waiting for +CMGS: ...]"));
}


// ESC (ASCII 27) throws away a half-typed message and gets you back out of
// the ">" prompt. Without it the only escape is resetting the module - and
// while ">" is showing, every AT command you type just becomes message text.
void sendEscape() {
  sim.write((char)27);
  Serial.println();
  Serial.println(F("[sent ESC - message abandoned, you are back at normal AT commands]"));
}


// Pulse RST low for 100ms to restart the module without unplugging it.
void resetModule() {
  Serial.println(F("[resetting module...]"));
  digitalWrite(SIM_RST_PIN, LOW);
  delay(100);
  digitalWrite(SIM_RST_PIN, HIGH);
  Serial.println(F("[reset done - give it ~10s to rejoin the network]"));
}
