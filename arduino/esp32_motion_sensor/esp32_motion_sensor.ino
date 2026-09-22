/*
  ============================================================
  ABMDMS - Wireless Motion Detection Monitoring System (ESP32)
  File   : esp32_motion_sensor.ino
  Board  : ESP32 Dev Module / NodeMCU-32S
  Sensors: 3x HC-SR501 PIR Motion Sensors (Room C, Room A, Room B)
  GSM    : SIM800L V2.2 (HardwareSerial UART2)
  Audio  : MAX98357A I2S 3W Class-D Amplifier + Speaker
  Wi-Fi  : Direct HTTP POST to XAMPP PHP API (No USB cord needed!)
  ============================================================

  HOW THIS WIRELESS SYSTEM WORKS:
  -------------------------------
  1. PIR sensor detects motion in Room A, B, or C.
  2. ESP32 plays non-blocking high-power siren through MAX98357A I2S speaker amp.
  3. ESP32 directly sends HTTP POST over Wi-Fi to your PC/Laptop running XAMPP:
        http://<PC_IP_ADDRESS>/ABMDMS/api/record_motion.php
  4. ESP32 queues and sends SMS via SIM800L (Hardware UART2 on GPIO 16/17).
  5. ESP32 sends SMS status over Wi-Fi to:
        http://<PC_IP_ADDRESS>/ABMDMS/api/record_sms.php
  
  * No PowerShell serial reader script needed!
  * No USB cable plugged into PC needed!
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include "driver/i2s.h"
#include <math.h>

// ============================================================
// SECTION 1 - WI-FI & SERVER SETTINGS (EDIT THESE)
// ============================================================

// 1. Enter your Wi-Fi credentials (2.4GHz network)
const char* WIFI_SSID     = "Kabit ni Francis";
const char* WIFI_PASSWORD = "qwerty123";

// 2. Enter your PC/Laptop Local IPv4 Address (find it using 'ipconfig' in cmd)
// Example: "192.168.1.15"
const char* SERVER_IP     = "10.192.10.14";
const int   SERVER_PORT   = 80;

// API Endpoints on your XAMPP server
String MOTION_API_URL = String("http://") + SERVER_IP + ":" + SERVER_PORT + "/ABMDMS/api/record_motion.php";
String SMS_API_URL    = String("http://") + SERVER_IP + ":" + SERVER_PORT + "/ABMDMS/api/record_sms.php";

// ============================================================
// SECTION 2 - SENSOR & PIN SETTINGS
// ============================================================

const int NUM_ZONES = 3;

// Safe ESP32 Input GPIOs for PIR Sensors (HC-SR501 OUT gives 3.3V logic)
const int   PIR_PIN[NUM_ZONES]   = { 13,      12,      14      }; 
const char* ZONE_NAME[NUM_ZONES] = { "ROOMC", "ROOMA", "ROOMB" }; 
const char* ZONE_TEXT[NUM_ZONES] = { "Room C","Room A","Room B" }; 

// Feedback Peripherals
const int LED_PIN = 2; // Built-in Blue LED on ESP32 DevKit (GPIO 2)

// MAX98357A I2S Amplifier Audio Pins
const int I2S_WS_PIN      = 25; // LRC / WS (Word Select) -> GPIO 25
const int I2S_BCK_PIN     = 26; // BCLK (Bit Clock)       -> GPIO 26
const int I2S_DATA_PIN    = 27; // DIN (Data In)          -> GPIO 27
const bool SPEAKER_ENABLED = true;

// Siren Settings for MAX98357A Speaker
const unsigned long SIREN_SWEEP_MS = 250;   // Speed of each high-low sweep (ms)
const int SIREN_CYCLES             = 4;     // Total sweeps per alarm trigger
const int SIREN_FREQ_LOW           = 600;   // Low pitch frequency (Hz)
const int SIREN_FREQ_HIGH          = 2200;  // High pitch frequency (Hz)
const int SIREN_VOLUME             = 18000; // Amplitude volume (1 to 32767)

const unsigned long WARMUP_SECONDS  = 30;   
const unsigned long STOP_CONFIRM_MS = 2000; 
const unsigned long START_CONFIRM_MS = 150; 

// ============================================================
// SECTION 3 - GSM / SIM800L SETTINGS
// ============================================================

const char* SMS_RECIPIENT = "+639169751409";
const bool  SIM_ENABLED   = true;

// ESP32 Hardware UART2 Pins (Direct & Fast)
const int SIM_RX_PIN  = 16;  // ESP32 GPIO 16 (RX2) <- SIM800L TXD
const int SIM_TX_PIN  = 17;  // ESP32 GPIO 17 (TX2) -> SIM800L RXD
const int SIM_RST_PIN = 4;   // ESP32 GPIO 4        -> SIM800L RST

HardwareSerial sim(2);       // Use Hardware UART2

const unsigned long SMS_COOLDOWN_MS = 60000;  // 60s per zone
const unsigned long SMS_MIN_GAP_MS  = 5000;   // 5s rest between sends
const unsigned long AT_TIMEOUT_MS   = 10000;  
const unsigned long CMGS_TIMEOUT_MS = 30000;  
const int SMS_MAX_FAILS_BEFORE_RESET = 3;     
const int MIN_SIGNAL = 10;                    

// ============================================================
// SECTION 4 - MEMORY & STATE VARIABLES
// ============================================================

bool          motionActive[NUM_ZONES] = {};   
unsigned long lowStartedAt[NUM_ZONES] = {};   
unsigned long highStartedAt[NUM_ZONES] = {};  

// Speaker state
bool          speakerPlaying = false;
unsigned long speakerStarted = 0;
float         i2sPhase       = 0.0f;

// SMS state
bool          smsPending[NUM_ZONES]  = {};   
unsigned long lastSmsAt[NUM_ZONES]   = {};   
bool          smsEverSent[NUM_ZONES] = {};   
int           smsZone = -1;   
bool          simReady   = false;   
int           smsFailRun = 0;       

enum SmsState {
  SMS_IDLE,
  SMS_WAIT_PROMPT,
  SMS_WAIT_CONFIRM
};

SmsState      smsState        = SMS_IDLE;
unsigned long smsStateSince   = 0;    
unsigned long smsLastFinished = 0;    

const int SIM_BUF_SIZE = 128;
char simBuf[SIM_BUF_SIZE];
int  simBufLen = 0;

// Wi-Fi Reconnect Timer
unsigned long lastWifiCheck = 0;

// Forward declarations
void postMotionEvent(const char* zone, const char* eventType);
void postSmsEvent(const char* zone, const char* status, const char* detail);
void simSetup();
void simReset();
void simDrain();
void simBufClear();
bool simSaw(const char* token);
bool simCommand(const char* command, const char* expect, unsigned long timeoutMs);
int  parseCsq();
void queueSms(int zone);
void smsTick();
void smsFinish(bool ok, const char* reason);
void i2sInit();
void speakerTick();
void speakerChirp(int freq, int durationMs);

// ============================================================
// SECTION 5 - SETUP
// ============================================================

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println(F("\n=========================================="));
  Serial.println(F("  ABMDMS - ESP32 Wireless Motion System   "));
  Serial.println(F("  Audio: MAX98357A I2S Speaker Amplifier  "));
  Serial.println(F("=========================================="));

  // Configure PIR Pins
  for (int i = 0; i < NUM_ZONES; i++) {
    pinMode(PIR_PIN[i], INPUT);
    Serial.printf("   PIR Pin %d -> %s\n", PIR_PIN[i], ZONE_NAME[i]);
  }

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Initialize MAX98357A I2S Driver
  i2sInit();

  // Initialize Wi-Fi Connection
  Serial.printf("Connecting to Wi-Fi SSID: %s", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - wifiStart < 15000) {
    delay(500);
    Serial.print(".");
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  }
  digitalWrite(LED_PIN, LOW);

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(F("\n[Wi-Fi] Connected successfully!"));
    Serial.print(F("[Wi-Fi] ESP32 IP Address: "));
    Serial.println(WiFi.localIP());
    Serial.print(F("[Server Target] "));
    Serial.println(MOTION_API_URL);
  } else {
    Serial.println(F("\n[Wi-Fi] Connection failed or timed out. Will auto-retry in background."));
  }

  // Initialize SIM800L
  simSetup();

  // PIR Warmup period
  Serial.printf("Warming up %d PIR sensors (%lu seconds)...\n", NUM_ZONES, WARMUP_SECONDS);
  for (unsigned long i = WARMUP_SECONDS; i > 0; i--) {
    Serial.printf("%lu...\n", i);
    delay(1000);
  }

  // Arming double chirp over speaker
  if (SPEAKER_ENABLED) {
    speakerChirp(1600, 80);
    delay(100);
    speakerChirp(2400, 120);
  }

  Serial.println(F("System Ready! Monitoring zones wirelessly..."));
}

// ============================================================
// SECTION 6 - MAIN LOOP
// ============================================================

void loop() {
  // Check Wi-Fi connection periodically
  if (millis() - lastWifiCheck > 10000) {
    lastWifiCheck = millis();
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println(F("[Wi-Fi] Disconnected, reconnecting..."));
      WiFi.reconnect();
    }
  }

  // 1. Poll each PIR zone
  for (int i = 0; i < NUM_ZONES; i++) {
    int sensorValue = digitalRead(PIR_PIN[i]);

    // CASE A: Motion Active
    if (sensorValue == HIGH) {
      lowStartedAt[i] = 0;

      if (motionActive[i] == false) {
        if (highStartedAt[i] == 0) {
          highStartedAt[i] = millis();
        }

        if (millis() - highStartedAt[i] >= START_CONFIRM_MS) {
          motionActive[i]  = true;
          highStartedAt[i] = 0;

          Serial.printf("[%s] MOTION DETECTED!\n", ZONE_NAME[i]);

          // Trigger audible speaker siren
          if (SPEAKER_ENABLED) {
            speakerPlaying = true;
            speakerStarted = millis();
            i2sPhase       = 0.0f;
          }

          // Send HTTP POST wirelessly to XAMPP backend
          postMotionEvent(ZONE_NAME[i], "MOTION_DETECTED");

          // Queue SMS alert
          queueSms(i);
        }
      }
    } 
    // CASE B: Motion Stopped
    else {
      highStartedAt[i] = 0;

      if (motionActive[i] == true) {
        if (lowStartedAt[i] == 0) {
          lowStartedAt[i] = millis();
        }

        if (millis() - lowStartedAt[i] >= STOP_CONFIRM_MS) {
          motionActive[i] = false;
          lowStartedAt[i] = 0;

          Serial.printf("[%s] MOTION STOPPED\n", ZONE_NAME[i]);

          // Send HTTP POST wirelessly to XAMPP backend
          postMotionEvent(ZONE_NAME[i], "MOTION_STOPPED");
        }
      }
    }
  }

  // Visual LED indicator
  bool anyActive = false;
  for (int i = 0; i < NUM_ZONES; i++) {
    if (motionActive[i]) {
      anyActive = true;
      break;
    }
  }
  digitalWrite(LED_PIN, anyActive ? HIGH : LOW);

  // Advance non-blocking tasks
  speakerTick();
  smsTick();

  delay(10);
}

// ============================================================
// SECTION 7 - HTTP POST OVER WI-FI TO PHP API
// ============================================================

void postMotionEvent(const char* zone, const char* eventType) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("[HTTP FAIL] Cannot send motion log - Wi-Fi not connected"));
    return;
  }

  HTTPClient http;
  http.begin(MOTION_API_URL);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.setTimeout(3000); // 3s timeout

  String postData = "event_type=" + String(eventType) +
                    "&zone=" + String(zone) +
                    "&source=ESP32_WIFI_PIR";

  int httpCode = http.POST(postData);

  if (httpCode > 0) {
    Serial.printf("[HTTP POST Motion] %s %s -> HTTP %d\n", zone, eventType, httpCode);
  } else {
    Serial.printf("[HTTP POST Motion Error] %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}

void postSmsEvent(const char* zone, const char* status, const char* detail) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("[HTTP FAIL] Cannot send SMS log - Wi-Fi not connected"));
    return;
  }

  HTTPClient http;
  http.begin(SMS_API_URL);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.setTimeout(3000);

  String postData = "zone=" + String(zone) +
                    "&status=" + String(status) +
                    "&detail=" + String(detail);

  int httpCode = http.POST(postData);

  if (httpCode > 0) {
    Serial.printf("[HTTP POST SMS Log] %s %s -> HTTP %d\n", zone, status, httpCode);
  } else {
    Serial.printf("[HTTP POST SMS Error] %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}

// ============================================================
// SECTION 8 - NON-BLOCKING MAX98357A I2S AUDIO DRIVER
// ============================================================

#define I2S_NUM            I2S_NUM_0
#define I2S_SAMPLE_RATE    22050
#define I2S_BUFFER_FRAMES  128

void i2sInit() {
  i2s_config_t i2s_config = {
    .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate          = I2S_SAMPLE_RATE,
    .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format       = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count        = 4,
    .dma_buf_len          = 256,
    .use_apll             = false,
    .tx_desc_auto_clear   = true
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num   = I2S_BCK_PIN,
    .ws_io_num    = I2S_WS_PIN,
    .data_out_num = I2S_DATA_PIN,
    .data_in_num  = I2S_PIN_NO_CHANGE
  };

  i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM, &pin_config);
  i2s_zero_dma_buffer(I2S_NUM);
}

void speakerChirp(int freq, int durationMs) {
  if (!SPEAKER_ENABLED) return;

  unsigned long start = millis();
  int16_t buffer[I2S_BUFFER_FRAMES * 2];
  float phase = 0.0f;
  float phaseInc = (2.0f * (float)M_PI * freq) / I2S_SAMPLE_RATE;
  size_t bytesWritten;

  while (millis() - start < (unsigned long)durationMs) {
    for (int i = 0; i < I2S_BUFFER_FRAMES; i++) {
      int16_t sample = (int16_t)(sin(phase) * SIREN_VOLUME);
      phase += phaseInc;
      if (phase >= 2.0f * (float)M_PI) phase -= 2.0f * (float)M_PI;
      buffer[i * 2]     = sample;
      buffer[i * 2 + 1] = sample;
    }
    i2s_write(I2S_NUM, buffer, sizeof(buffer), &bytesWritten, portMAX_DELAY);
  }

  // Clear buffer so no hum/buzz remains
  memset(buffer, 0, sizeof(buffer));
  i2s_write(I2S_NUM, buffer, sizeof(buffer), &bytesWritten, portMAX_DELAY);
}

void speakerTick() {
  if (!SPEAKER_ENABLED || !speakerPlaying) {
    return;
  }

  unsigned long elapsed = millis() - speakerStarted;
  unsigned long totalDuration = SIREN_SWEEP_MS * SIREN_CYCLES;

  if (elapsed >= totalDuration) {
    speakerPlaying = false;
    int16_t zeroBuf[I2S_BUFFER_FRAMES * 2] = {0};
    size_t bytesWritten;
    i2s_write(I2S_NUM, zeroBuf, sizeof(zeroBuf), &bytesWritten, 0);
    return;
  }

  unsigned long cycleTime = elapsed % SIREN_SWEEP_MS;
  unsigned long halfCycle = SIREN_SWEEP_MS / 2;
  int freq;

  if (cycleTime < halfCycle) {
    freq = SIREN_FREQ_LOW + (int)(((long)(SIREN_FREQ_HIGH - SIREN_FREQ_LOW) * cycleTime) / halfCycle);
  } else {
    unsigned long downTime = cycleTime - halfCycle;
    freq = SIREN_FREQ_HIGH - (int)(((long)(SIREN_FREQ_HIGH - SIREN_FREQ_LOW) * downTime) / halfCycle);
  }

  float phaseInc = (2.0f * (float)M_PI * freq) / I2S_SAMPLE_RATE;
  int16_t buffer[I2S_BUFFER_FRAMES * 2];

  for (int i = 0; i < I2S_BUFFER_FRAMES; i++) {
    int16_t sample = (int16_t)(sin(i2sPhase) * SIREN_VOLUME);
    i2sPhase += phaseInc;
    if (i2sPhase >= 2.0f * (float)M_PI) i2sPhase -= 2.0f * (float)M_PI;
    buffer[i * 2]     = sample;
    buffer[i * 2 + 1] = sample;
  }

  size_t bytesWritten;
  i2s_write(I2S_NUM, buffer, sizeof(buffer), &bytesWritten, 0);
}

// ============================================================
// SECTION 9 - SMS STATE MACHINE & HARDWARE UART
// ============================================================

void queueSms(int zone) {
  if (!SIM_ENABLED) return;

  if (smsEverSent[zone] && (millis() - lastSmsAt[zone] < SMS_COOLDOWN_MS)) {
    Serial.printf("[SMS SKIP] %s (Cooldown)\n", ZONE_NAME[zone]);
    postSmsEvent(ZONE_NAME[zone], "SKIPPED", "COOLDOWN");
    return;
  }

  if (!simReady) {
    Serial.printf("[SMS FAIL] %s (No SIM module)\n", ZONE_NAME[zone]);
    postSmsEvent(ZONE_NAME[zone], "FAILED", "NOMODULE");
    return;
  }

  smsPending[zone] = true;
}

void smsTick() {
  if (!SIM_ENABLED) return;

  switch (smsState) {
    case SMS_IDLE: {
      if (!simReady) return;

      int next = -1;
      for (int i = 0; i < NUM_ZONES; i++) {
        if (!smsPending[i]) continue;
        if (smsEverSent[i] && (millis() - lastSmsAt[i] < SMS_COOLDOWN_MS)) {
          smsPending[i] = false;
          postSmsEvent(ZONE_NAME[i], "SKIPPED", "COOLDOWN");
          continue;
        }
        next = i;
        break;
      }
      if (next < 0) return;

      if (smsLastFinished != 0 && (millis() - smsLastFinished < SMS_MIN_GAP_MS)) {
        return;
      }

      smsPending[next] = false;
      smsZone          = next;

      simBufClear();
      sim.print("AT+CMGS=\"");
      sim.print(SMS_RECIPIENT);
      sim.print("\"\r");

      smsState      = SMS_WAIT_PROMPT;
      smsStateSince = millis();
      break;
    }

    case SMS_WAIT_PROMPT: {
      simDrain();

      if (simSaw(">")) {
        sim.print("ABMDMS ALERT: Motion detected in ");
        sim.print(ZONE_TEXT[smsZone]);
        sim.print(" (");
        sim.print(ZONE_NAME[smsZone]);
        sim.print("). Uptime ");
        sim.print(millis() / 60000UL);
        sim.print(" min.");
        sim.write(26); // Ctrl+Z

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

void smsFinish(bool ok, const char* reason) {
  if (!ok) {
    sim.write((char) 27); // ESC
    sim.print("\r");
  }

  int zone = (smsZone >= 0 && smsZone < NUM_ZONES) ? smsZone : 0;
  lastSmsAt[zone]   = millis();
  smsEverSent[zone] = true;

  if (ok) {
    Serial.printf("[SMS SENT] %s\n", ZONE_NAME[zone]);
    postSmsEvent(ZONE_NAME[zone], "SENT", "");
  } else {
    Serial.printf("[SMS FAIL] %s: %s\n", ZONE_NAME[zone], reason);
    postSmsEvent(ZONE_NAME[zone], "FAILED", reason);
  }

  smsFailRun = ok ? 0 : (smsFailRun + 1);
  smsState        = SMS_IDLE;
  smsZone         = -1;
  smsLastFinished = millis();
  simBufClear();

  if (smsFailRun >= SMS_MAX_FAILS_BEFORE_RESET) {
    smsFailRun = 0;
    simReset();
  }
}

// ============================================================
// SECTION 10 - SIM800L HARDWARE INTERACTION
// ============================================================

void simBufClear() {
  simBufLen = 0;
  simBuf[0] = '\0';
}

void simDrain() {
  while (sim.available()) {
    char c = (char) sim.read();
    if (c == '\0') continue;

    if (simBufLen >= SIM_BUF_SIZE - 1) {
      int keep = SIM_BUF_SIZE / 2;
      memmove(simBuf, simBuf + (simBufLen - keep), keep);
      simBufLen = keep;
    }
    simBuf[simBufLen++] = c;
    simBuf[simBufLen]   = '\0';
  }
}

int parseCsq() {
  char* at = strstr(simBuf, "+CSQ:");
  if (at == NULL) return -1;
  return atoi(at + 5);
}

bool simSaw(const char* token) {
  return strstr(simBuf, token) != NULL;
}

bool simCommand(const char* command, const char* expect, unsigned long timeoutMs) {
  simBufClear();
  sim.print(command);
  sim.print("\r");

  unsigned long startedAt = millis();
  while (millis() - startedAt < timeoutMs) {
    simDrain();
    if (simSaw(expect)) return true;
    if (simSaw("ERROR")) return false;
  }
  return false;
}

void simSetup() {
  if (!SIM_ENABLED) {
    Serial.println(F("[SIM] Disabled in settings"));
    return;
  }

  pinMode(SIM_RST_PIN, OUTPUT);
  digitalWrite(SIM_RST_PIN, HIGH);

  // Start Hardware UART2 on GPIO 16 (RX), GPIO 17 (TX)
  sim.begin(9600, SERIAL_8N1, SIM_RX_PIN, SIM_TX_PIN);
  Serial.println(F("Starting SIM800L on Hardware UART2..."));
  delay(3000);

  if (!simCommand("AT", "OK", 5000)) {
    Serial.println(F("[SIM FAIL] No response to AT. Check power, common GND, RX/TX."));
    simReady = false;
    return;
  }

  simCommand("ATE0", "OK", 3000);
  simCommand("AT+CMEE=2", "OK", 3000);

  if (!simCommand("AT+CPIN?", "READY", 5000)) {
    Serial.println(F("[SIM FAIL] SIM card missing or locked."));
    simReady = false;
    return;
  }

  simCommand("AT+CSQ", "+CSQ", 5000);
  int signal = parseCsq();
  Serial.printf("[SIM] Signal quality: %d\n", signal);

  bool registered = simCommand("AT+CREG?", ",1", 10000);
  if (!registered) {
    registered = simCommand("AT+CREG?", ",5", 10000);
  }
  if (!registered) {
    Serial.println(F("[SIM FAIL] Network registration failed."));
    simReady = false;
    return;
  }

  if (!simCommand("AT+CMGF=1", "OK", 5000)) {
    Serial.println(F("[SIM FAIL] Could not set text mode."));
    simReady = false;
    return;
  }

  simReady = true;
  Serial.printf("[SIM READY] Alerts will be sent to %s\n", SMS_RECIPIENT);
}

void simReset() {
  Serial.println(F("[SIM RESET] Resetting SIM800L module..."));
  digitalWrite(SIM_RST_PIN, LOW);
  delay(150);
  digitalWrite(SIM_RST_PIN, HIGH);
  delay(3000);

  simBufClear();
  simReady = simCommand("AT", "OK", 5000);
  if (simReady) {
    simCommand("ATE0", "OK", 3000);
    simReady = simCommand("AT+CMGF=1", "OK", 5000);
  }
  smsLastFinished = millis();
}
