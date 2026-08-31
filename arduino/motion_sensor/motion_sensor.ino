/*
  ============================================================
  ABMDMS - Arduino Based Motion Detection Monitoring System
  File   : motion_sensor.ino
  Board  : Arduino Uno
  Sensors: 3x HC-SR501 PIR Motion Sensor  (Pins 2, 3, 4)
  GSM    : SIM800L V2.2 (UNV, the 5V board) - texts on motion
  ============================================================

  This is the live system's sketch. The pir_sms_test folder holds
  an identical rehearsal copy that writes to its own database, so
  test any change there first - then bring it here.

      3x PIR -> Arduino -> SIM800L -> your phone
                        -> USB -> PowerShell -> PHP -> MySQL -> dashboard


  WIRING - POWER & SENSORS   (see ../wiring.html, or arduino/PIR_MULTI_ZONE_WIRING.md)
  ------------------------
      HW-131 5V (from 12V adapter) -> Breadboard (+) rail -> Arduino 5V Pin (powers Arduino)
      HW-131 GND                   -> Breadboard (-) rail -> Arduino GND Pin
      USB Cable                    -> Connected to PC/Laptop (Serial Data Link only)

      Arduino Pin 2  <-  PIR 1 OUT   ->  zone ROOMC  ("Room C")
      Arduino Pin 3  <-  PIR 2 OUT   ->  zone ROOMA  ("Room A")
      Arduino Pin 4  <-  PIR 3 OUT   ->  zone ROOMB  ("Room B")
      Arduino Pin 8  ->  5V Piezo Buzzer (+) leg (audible detection alarm)

      Arduino Pin 5      Room D - REMOVED. The pin would not respond
                         to two different sensors. See SECTION 1 for
                         how to put it back once that is fixed.

  Every PIR OUT gets its OWN wire to its OWN pin - never share one.
  All PIR VCC ->  breadboard (+) rail  ->  HW-131 5V
  All PIR GND ->  breadboard (-) rail  ->  HW-131 GND / Arduino GND

  The pin/zone order looks odd on purpose: Pin 2 is ROOMC because
  that was the very first sensor built, and the mapping was kept
  when the other rooms were added.

  On each sensor: yellow jumper on H (retrigger), time-delay screw
  turned fully anti-clockwise (shortest). The V/O/G pin order under
  the dome varies between batches - read the silkscreen.

  WIRING - SIM800L V2.2  (see ../wiring.html, or arduino/SIM800L_WIRING.md)
  ---------------------
  SIM800L TXD  ->  Arduino Pin 10       (direct)
  SIM800L RXD  ->  Arduino Pin 11       (direct - V2.2 is a 5V-logic
                                         board, so NO 1k/2k divider)
  SIM800L RST  ->  Arduino Pin 12       (optional, used to recover)
  SIM800L 5Vin ->  HW-131 5V Top Rail + (12V wall adapter input)
  SIM800L GND  ->  HW-131 GND Top Rail - AND Arduino GND (must be common)
  SIM800L VDD  ->  leave UNCONNECTED    (it is a 2.8V reference
                                         OUTPUT, not a power input.
                                         Feeding it 5V kills the board.)
  1000uF capacitor across the module's 5Vin / GND, in PARALLEL.
  Watch the polarity: the striped leg goes to GND.


  WHAT THIS PROGRAM DOES
  ----------------------
  1. Wakes the SIM800L and checks it can actually text.
  2. Waits 30 seconds for the PIRs to warm up (they lie at first).
  3. Prints "ROOMA_MOTION_DETECTED" once when movement STARTS
     in that room - and the same for ROOMB and ROOMC.
  4. Prints "ROOMA_MOTION_STOPPED"  once when movement ENDS.
  5. Texts your phone when movement STARTS (never when it stops),
     then prints "SMS_SENT:ROOMA" / "SMS_FAIL:ROOMA:<REASON>" /
     "SMS_SKIP:ROOMA:COOLDOWN" so the laptop can log the result.

  IMPORTANT: it only prints when the state CHANGES. Printing every
  loop would flood the database with thousands of rows per second.

  IMPORTANT: each room is watched completely separately. Room A
  being busy does not hide Room B, and the 60-second text cooldown
  is counted PER ROOM - so movement in a second room still texts
  you straight away even if the first room just did.

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

// The rooms. These three lists line up: position 0 of each
// describes the SAME sensor. To add or move a room, edit all three
// and update NUM_ZONES - nothing else in the sketch needs changing.
const int NUM_ZONES = 3;

const int   PIR_PIN[NUM_ZONES]   = {  2,       3,       4      }; // OUT wire
const char* ZONE_NAME[NUM_ZONES] = { "ROOMC", "ROOMA", "ROOMB" }; // in the event tokens
const char* ZONE_TEXT[NUM_ZONES] = { "Room C","Room A","Room B" }; // inside the SMS

// --- ROOM D IS REMOVED, NOT DELETED ---------------------------
// Pin 5 would not respond to two different sensors, so it is out of
// the system until that is fixed. Since two sensors both failed on
// it, suspect the pin, the OUT wire or its rail tap - not the sensors.
//
// To put Room D back once Pin 5 works: set NUM_ZONES to 4 and add
// the fourth value to each of the three lists above -
//
//     PIR_PIN    ...,  5
//     ZONE_NAME  ..., "ROOMD"
//     ZONE_TEXT  ..., "Room D"
//
// then add 'ROOMD' back to ALLOWED_ZONES and ZONE_LABELS in
// config.php, and to the two regexes in serial/serial_reader.ps1
// AND serial/serial_reader.php. Nothing else needs touching.
// --------------------------------------------------------------

const int LED_PIN    = 13;             // Built-in LED, lights while ANY room is active
const int BUZZER_PIN = 8;              // 5V Passive Piezo Buzzer (+) leg -> Arduino Pin 8
const bool BUZZER_ENABLED = true;      // Set false to mute buzzer

// Siren / Chirp settings for passive buzzer:
// 4 cycles of "piwiw" (each 200ms = 100ms sweep up + 100ms sweep down) = 800ms total
const unsigned long BUZZER_SWEEP_MS = 200; // Duration of one "piwiw" cycle (ms)
const int BUZZER_CYCLES             = 4;   // 4 repetitions: "piwiw piwiw piwiw piwiw"
const int BUZZER_FREQ_LOW           = 800;  // Starting/ending pitch (Hz)
const int BUZZER_FREQ_HIGH          = 2500; // Peak pitch (Hz)

const unsigned long WARMUP_SECONDS  = 30;    // PIR warm-up time
const unsigned long BAUD_RATE       = 9600;  // Must match the Serial Monitor
const unsigned long STOP_CONFIRM_MS = 2000;  // Quiet for this long = motion really stopped

// A pin must stay HIGH for this long before we believe it.
//
// The SIM800L pulls about 2 A in short bursts while it transmits,
// and radiates hard from its antenna. Both of those can put a
// false pulse on a PIR's OUT wire, which the Arduino reads as a
// person. That is why a room can appear to trigger at the exact
// moment another room's text is going out.
//
// A real PIR holds its output HIGH for seconds. Interference is
// far shorter, so waiting a fraction of a second before believing
// a HIGH throws the interference away and costs nothing real.
//
// If a false room STILL appears with this set, the burst is not
// coupling into the wire - it is triggering the sensor itself, and
// only moving the antenna or the sensor will fix it.
const unsigned long START_CONFIRM_MS = 150;  // 0 disables this filter


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

// Never send more than one SMS per ROOM inside this window. This is
// what stops one person pacing around from draining your load.
// Counted separately for each room, so a real intruder moving from
// Room A to Room B still gets you a second text immediately.
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

// One slot per room, in the same order as the lists above.
// "= {}" fills every slot with false / 0 whatever NUM_ZONES is, so
// adding or removing a room never means counting braces here.
bool          motionActive[NUM_ZONES] = {};   // already announced?
unsigned long lowStartedAt[NUM_ZONES] = {};   // when it went quiet
unsigned long highStartedAt[NUM_ZONES] = {};  // when it first went busy

// --- Passive Buzzer Siren Memory (Non-blocking) ---
bool          buzzerPlaying = false;
unsigned long buzzerStarted = 0;

// --- SMS memory ---
SoftwareSerial sim(SIM_RX_PIN, SIM_TX_PIN);

bool          smsPending[NUM_ZONES]  = {};   // text waiting for this room
unsigned long lastSmsAt[NUM_ZONES]   = {};   // when we last texted about it
bool          smsEverSent[NUM_ZONES] = {};   // EVER texted about it?
                                      // (millis() starts at 0, so without this
                                      //  flag the first alert is blocked)

int smsZone = -1;   // which room the text now going out belongs to

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

  for (int i = 0; i < NUM_ZONES; i++) {
    pinMode(PIR_PIN[i], INPUT);
  }
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  pinMode(BUZZER_PIN, OUTPUT);
  noTone(BUZZER_PIN);

  Serial.print(F("ABMDMS - "));
  Serial.print(NUM_ZONES);
  Serial.println(F(" sensors, 1 SIM800L, 1 Passive Buzzer"));
  for (int i = 0; i < NUM_ZONES; i++) {
    Serial.print(F("   Pin "));
    Serial.print(PIR_PIN[i]);
    Serial.print(F(" -> "));
    Serial.println(ZONE_NAME[i]);
  }
  Serial.print(F("   Pin "));
  Serial.print(BUZZER_PIN);
  Serial.println(F(" -> 5V Passive Buzzer (piwiw siren)"));

  // Wake the GSM module FIRST. It needs time to find the network,
  // and it can do that while the PIR sensor warms up.
  simSetup();

  // The HC-SR501 gives false readings for the first few seconds
  // after power on. Wait it out, with a countdown so you can see
  // the system is not frozen. They all warm up together, so stay
  // away from ALL of them until it says "System Ready".
  Serial.print(F("Warming up "));
  Serial.print(NUM_ZONES);
  Serial.print(F(" PIR sensors, please stay still ("));
  Serial.print(WARMUP_SECONDS);
  Serial.println(F(" seconds)..."));

  for (unsigned long i = WARMUP_SECONDS; i > 0; i--) {
    Serial.print(i);
    Serial.println(F("..."));
    delay(1000);
  }

  // Play arming chirp (high double tone) to signal warmup complete
  if (BUZZER_ENABLED) {
    tone(BUZZER_PIN, 1800, 80);
    delay(100);
    tone(BUZZER_PIN, 2500, 120);
    delay(140);
    noTone(BUZZER_PIN);
  }

  Serial.println(F("System Ready"));
  Serial.println(F("Waiting for motion..."));
}


// ============================================================
// SECTION 4 - MAIN LOOP
// Runs over and over again, forever.
// ============================================================

void loop() {

  // Check every room, one after the other. Each keeps its own
  // memory, so what happens in one never affects another.
  for (int i = 0; i < NUM_ZONES; i++) {

    // Read this room's sensor. HIGH = movement, LOW = no movement.
    int sensorValue = digitalRead(PIR_PIN[i]);


    // --------------------------------------------------------
    // CASE A: this sensor sees movement
    // --------------------------------------------------------
    if (sensorValue == HIGH) {

      // Movement is still happening, so cancel any "stop" countdown
      lowStartedAt[i] = 0;

      // Only announce it if we have NOT already announced it.
      // This is what stops duplicate messages.
      if (motionActive[i] == false) {

        // Note when this pin FIRST went HIGH, then make it hold that
        // for START_CONFIRM_MS before believing it. A real person
        // keeps the output HIGH for seconds; a burst of electrical
        // noise from the SIM800L does not last anything like that.
        if (highStartedAt[i] == 0) {
          highStartedAt[i] = millis();
        }

        if (millis() - highStartedAt[i] >= START_CONFIRM_MS) {
          motionActive[i]  = true;
          highStartedAt[i] = 0;

          Serial.print(ZONE_NAME[i]);
          Serial.println(F("_MOTION_DETECTED"));   // <-- the laptop reads this line

          // Trigger "piwiw piwiw piwiw piwiw" siren (Non-blocking)
          if (BUZZER_ENABLED) {
            buzzerPlaying = true;
            buzzerStarted = millis();
          }

          // Ask for an SMS. It is NOT sent here - it is put in line
          // and sent a small piece at a time by smsTick().
          queueSms(i);
        }
      }
    }


    // --------------------------------------------------------
    // CASE B: this sensor sees nothing
    // --------------------------------------------------------
    else {

      // The pin dropped again. If it never held HIGH long enough to
      // be believed, forget it completely - that was the noise this
      // filter exists to throw away.
      highStartedAt[i] = 0;

      // Only care if motion was previously happening here
      if (motionActive[i] == true) {

        // Start a small timer the first moment it goes quiet. The
        // PIR output flickers, so wait a couple of seconds to be
        // sure the person really left.
        if (lowStartedAt[i] == 0) {
          lowStartedAt[i] = millis();
        }

        if (millis() - lowStartedAt[i] >= STOP_CONFIRM_MS) {
          motionActive[i] = false;
          lowStartedAt[i] = 0;

          Serial.print(ZONE_NAME[i]);
          Serial.println(F("_MOTION_STOPPED"));  // <-- the laptop reads this line
          // NOTE: no SMS here on purpose. Only the START of motion texts.
        }
      }
    }
  }


  // The one built-in LED has to stand for every room, so it
  // means "somebody is somewhere" - lit while ANY room is active.
  bool anyActive = false;
  for (int i = 0; i < NUM_ZONES; i++) {
    if (motionActive[i]) {
      anyActive = true;
      break;
    }
  }
  digitalWrite(LED_PIN, anyActive ? HIGH : LOW);

  // Advance "piwiw piwiw piwiw piwiw" siren frequency without blocking
  buzzerTick();

  // Move any in-progress SMS forward by one small step.
  // This returns immediately - it never waits.
  smsTick();

  // Small pause so we do not read the pin millions of times per second
  delay(10);
}


// ============================================================
// SECTION 4B - NON-BLOCKING "PIWIW" BUZZER SIREN
// Sweeps pitch up and down rapidly in repeating cycles.
// ============================================================

void buzzerTick() {
  if (!BUZZER_ENABLED || !buzzerPlaying) {
    return;
  }

  unsigned long elapsed = millis() - buzzerStarted;
  unsigned long totalDuration = BUZZER_SWEEP_MS * BUZZER_CYCLES;

  if (elapsed >= totalDuration) {
    buzzerPlaying = false;
    noTone(BUZZER_PIN);
    return;
  }

  // Position within current 200ms cycle (0..199)
  unsigned long cycleTime = elapsed % BUZZER_SWEEP_MS;
  unsigned long halfCycle = BUZZER_SWEEP_MS / 2; // 100ms
  int freq;

  if (cycleTime < halfCycle) {
    // Pitch sweep UP (e.g. 800 Hz -> 2500 Hz over 100ms)
    freq = BUZZER_FREQ_LOW + (int)(((long)(BUZZER_FREQ_HIGH - BUZZER_FREQ_LOW) * cycleTime) / halfCycle);
  } else {
    // Pitch sweep DOWN (e.g. 2500 Hz -> 800 Hz over 100ms)
    unsigned long downTime = cycleTime - halfCycle;
    freq = BUZZER_FREQ_HIGH - (int)(((long)(BUZZER_FREQ_HIGH - BUZZER_FREQ_LOW) * downTime) / halfCycle);
  }

  tone(BUZZER_PIN, freq);
}


// ============================================================
// SECTION 5 - SMS: PUTTING A MESSAGE IN LINE
// ============================================================

// Called the moment one room's sensor starts seeing movement.
// "zone" is that room's position in the lists at the top.
void queueSms(int zone) {

  if (!SIM_ENABLED) {
    return;
  }

  // Cooldown check: did we text about THIS ROOM recently?
  // smsEverSent tells the difference between "texted a long time
  // ago" and "never texted at all".
  if (smsEverSent[zone] && (millis() - lastSmsAt[zone] < SMS_COOLDOWN_MS)) {
    Serial.print(F("SMS_SKIP:"));
    Serial.print(ZONE_NAME[zone]);
    Serial.println(F(":COOLDOWN"));
    return;
  }

  if (!simReady) {
    Serial.print(F("SMS_FAIL:"));
    Serial.print(ZONE_NAME[zone]);
    Serial.println(F(":NOMODULE"));
    return;
  }

  smsPending[zone] = true;
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

      if (!simReady) {
        return;
      }

      // Is any room waiting for a text? Take the first one we find.
      // The others keep their turn and go out on a later pass - the
      // module can only send one message at a time.
      //
      // Re-check each room's cooldown HERE, right before sending.
      // Checking it only in queueSms() is not enough: sending takes
      // 5-10 seconds, and a room can easily be triggered again in
      // that time. That second request is queued while the first
      // text is still in flight - and at that moment no text has
      // finished for that room yet, so the cooldown legitimately
      // does not apply and the request is accepted. It then sits in
      // smsPending[] and fires the instant the first send completes,
      // sending a SECOND text for what should have been one alert.
      // By the time we get here the first text HAS finished, so the
      // cooldown is live and the stale request is dropped.
      int next = -1;
      for (int i = 0; i < NUM_ZONES; i++) {
        if (!smsPending[i]) {
          continue;
        }
        if (smsEverSent[i] && (millis() - lastSmsAt[i] < SMS_COOLDOWN_MS)) {
          smsPending[i] = false;
          Serial.print(F("SMS_SKIP:"));
          Serial.print(ZONE_NAME[i]);
          Serial.println(F(":COOLDOWN"));
          continue;         // that room is muted; keep looking at the others
        }
        next = i;
        break;
      }
      if (next < 0) {
        return;
      }

      // Give the module its rest before starting another message.
      if (smsLastFinished != 0 && (millis() - smsLastFinished < SMS_MIN_GAP_MS)) {
        return;   // the request stays pending; we will come back to it
      }

      smsPending[next] = false;
      smsZone          = next;   // remember it, so the result is credited right

      // Tell the module who we are texting. It should answer ">".
      simBufClear();
      sim.print(F("AT+CMGS=\""));
      sim.print(SMS_RECIPIENT);
      sim.print(F("\"\r"));

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
        sim.print(F("ABMDMS ALERT: Motion detected in "));
        sim.print(ZONE_TEXT[smsZone]);
        sim.print(F(" ("));
        sim.print(ZONE_NAME[smsZone]);
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
    sim.print(F("\r"));
  }

  // Credit the result to the room this message belonged to, so only
  // that room goes on cooldown. smsZone should always be valid here,
  // but guard anyway - a stray index would corrupt another room's
  // memory instead of failing loudly.
  int zone = (smsZone >= 0 && smsZone < NUM_ZONES) ? smsZone : 0;

  lastSmsAt[zone]   = millis();
  smsEverSent[zone] = true;

  if (ok) {
    Serial.print(F("SMS_SENT:"));
    Serial.println(ZONE_NAME[zone]);
  } else {
    Serial.print(F("SMS_FAIL:"));
    Serial.print(ZONE_NAME[zone]);
    Serial.print(F(":"));
    Serial.println(reason);
  }

  smsFailRun = ok ? 0 : (smsFailRun + 1);

  smsState        = SMS_IDLE;
  smsZone         = -1;
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
// Prints SIM_READY or SIM_FAIL:<reason> for the serial bridge.
void simSetup() {

  if (!SIM_ENABLED) {
    Serial.println(F("SIM_FAIL:DISABLED"));
    return;
  }

  pinMode(SIM_RST_PIN, OUTPUT);
  digitalWrite(SIM_RST_PIN, HIGH);   // RST is active LOW - keep it released

  sim.begin(SIM_BAUD_RATE);
  Serial.println(F("Starting SIM800L, please wait..."));
  delay(3000);                       // the module needs a moment after power on

  // 1. Is it alive?  ->  "OK"
  if (!simCommand("AT", "OK", 5000)) {
    Serial.println(F("SIM_FAIL:NOREPLY"));
    Serial.println(F("   Check: external power, common GND, TX/RX not swapped."));
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
    Serial.println(F("SIM_FAIL:NOSIM"));
    Serial.println(F("   Check: SIM inserted properly, PIN lock turned OFF."));
    simReady = false;
    return;
  }

  // 5. How strong is the signal?
  //    0-31, bigger is better. 99 means "no signal at all".
  //    Under 10 is too weak to send reliably.
  simCommand("AT+CSQ", "+CSQ", 5000);
  int signal = parseCsq();
  Serial.print(F("   Signal: "));
  if (signal < 0) {
    Serial.println(F("could not read"));
  } else if (signal == 99) {
    Serial.println(F("99 - NO SIGNAL (check the antenna)"));
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
    Serial.println(F("SIM_FAIL:NONETWORK"));
    Serial.println(F("   Check: antenna, load/credit, and 2G coverage in this area."));
    simReady = false;
    return;
  }

  // 7. Use plain text messages (not the binary PDU format)
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


// Hard-reset the module by pulling its RST pin low for a moment.
// Used when several sends fail in a row.
void simReset() {
  Serial.println(F("SIM_RESET"));

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
