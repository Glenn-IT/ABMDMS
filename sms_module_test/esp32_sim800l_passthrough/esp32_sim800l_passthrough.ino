/*
  ============================================================
  ESP32 SIM800L PASSTHROUGH (AT Command Test Tool)
  File  : esp32_sim800l_passthrough.ino
  Board : ESP32 Dev Module
  Port  : Hardware UART2 (GPIO 16 RX2 / GPIO 17 TX2)
  ============================================================

  WHAT THIS DOES:
  ---------------
  Allows you to type AT commands directly into the Serial Monitor
  to test if your SIM800L module is receiving power, reading the SIM,
  connecting to the 2G cell tower, and sending SMS.

  WIRING (ESP32):
  ---------------
  SIM800L TXD  ->  ESP32 GPIO 16 (RX2)
  SIM800L RXD  ->  ESP32 GPIO 17 (TX2)
  SIM800L RST  ->  ESP32 GPIO 4
  SIM800L 5Vin ->  HW-131 5V Rail (with 12V wall adapter)
  SIM800L GND  ->  HW-131 GND AND ESP32 GND (Common Ground)
  1000uF capacitor across 5Vin and GND (stripe to GND).
  Antenna screwed on firmly.

  SERIAL MONITOR SETTINGS:
  ------------------------
  - Baud rate: 115200
  - Line ending dropdown: "Both NL & CR"
*/

#include <Arduino.h>

const int SIM_RX_PIN  = 16;  // ESP32 GPIO 16 <- SIM800L TXD
const int SIM_TX_PIN  = 17;  // ESP32 GPIO 17 -> SIM800L RXD
const int SIM_RST_PIN = 4;   // ESP32 GPIO 4  -> SIM800L RST

HardwareSerial sim(2); // Hardware UART2

String typed = "";
bool lineJustEnded = false;

void sendCtrlZ();
void sendEscape();
void resetModule();
void sendLine();

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(SIM_RST_PIN, OUTPUT);
  digitalWrite(SIM_RST_PIN, HIGH); // Active LOW

  sim.begin(9600, SERIAL_8N1, SIM_RX_PIN, SIM_TX_PIN);

  Serial.println(F("\n============================================"));
  Serial.println(F("ESP32 SIM800L PASSTHROUGH TEST TOOL"));
  Serial.println(F("============================================"));
  Serial.println(F("Set Serial Monitor line ending to 'Both NL & CR'"));
  Serial.println(F("Baud Rate: 115200"));
  Serial.println(F(""));
  Serial.println(F("Step-by-step Test Commands:"));
  Serial.println(F("  1. AT             -> Expect 'OK' (Module is alive)"));
  Serial.println(F("  2. AT+CMEE=2      -> Expect 'OK' (Detailed error messages)"));
  Serial.println(F("  3. AT+CPIN?       -> Expect '+CPIN: READY' (SIM is readable)"));
  Serial.println(F("  4. AT+CSQ         -> Expect '+CSQ: X,0' (Signal strength, >10 is good)"));
  Serial.println(F("  5. AT+CREG?       -> Expect '+CREG: 0,1' (Registered on network)"));
  Serial.println(F("  6. AT+CMGF=1      -> Expect 'OK' (Text mode)"));
  Serial.println(F("  7. AT+CMGS=\"+639XXXXXXXXX\" -> Expect '>' prompt"));
  Serial.println(F("     (Type your test message, then type: END)"));
  Serial.println(F(""));
  Serial.println(F("Helper Commands:"));
  Serial.println(F("  END    -> Sends Ctrl+Z to send the SMS"));
  Serial.println(F("  CANCEL -> Sends ESC to cancel a stuck message"));
  Serial.println(F("  RESET  -> Pulses RST pin to reboot SIM800L"));
  Serial.println(F("--------------------------------------------\n"));
}

void loop() {
  // Read from SIM800L and print to PC Serial Monitor
  while (sim.available()) {
    Serial.write(sim.read());
  }

  // Read from PC Serial Monitor and send to SIM800L
  while (Serial.available()) {
    char c = Serial.read();

    if (c == '\r' || c == '\n') {
      if (lineJustEnded) { 
        lineJustEnded = false; 
        continue; 
      }
      sendLine();
      lineJustEnded = true;
    } else {
      lineJustEnded = false;
      if (typed.length() < 200) typed += c;
    }
  }
}

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

void sendCtrlZ() {
  sim.write((char)26);
  Serial.println();
  Serial.println(F("[sent Ctrl+Z - waiting for +CMGS: confirmation...]"));
}

void sendEscape() {
  sim.write((char)27);
  Serial.println();
  Serial.println(F("[sent ESC - message abandoned, returned to AT mode]"));
}

void resetModule() {
  Serial.println(F("[Resetting SIM800L...]"));
  digitalWrite(SIM_RST_PIN, LOW);
  delay(150);
  digitalWrite(SIM_RST_PIN, HIGH);
  Serial.println(F("[Reset complete - wait ~10s to rejoin network]"));
}
