# ABMDMS — Integration & Deployment Checklist

Use this checklist to track progress when setting up, testing, and validating the **ABMDMS PIR Sensor & System Features**. Work through each phase sequentially.

---

## Phase 1 — Hardware Inspection & Circuit Wiring

- [ ] **PIR Sensors (HC-SR501)**
  - [ ] Connect Sensor 1 VCC -> Breadboard 5V Rail, GND -> Ground Rail, OUT -> Arduino **Pin 2** (`ROOMC`)
  - [ ] Connect Sensor 2 VCC -> Breadboard 5V Rail, GND -> Ground Rail, OUT -> Arduino **Pin 3** (`ROOMA`)
  - [ ] Connect Sensor 3 VCC -> Breadboard 5V Rail, GND -> Ground Rail, OUT -> Arduino **Pin 4** (`ROOMB`)
  - [ ] *(Optional 4th Zone)* Connect Sensor 4 OUT -> Arduino **Pin 5** (`ROOMD`)
  - [ ] Confirm yellow jumper on each PIR sensor is set to **H** position (Retriggerable mode)
  - [ ] Turn time-delay potentiometer fully **counter-clockwise** on all sensors

- [ ] **GSM Module (SIM800L V2.2)**
  - [ ] Connect SIM800L 5Vin to **External 5V/2A Power Supply (+)**
  - [ ] Connect SIM800L GND to **External Power Supply (-)** AND **Arduino GND** (Common Ground)
  - [ ] Connect 1000µF capacitor across 5Vin and GND in parallel (observe polarity: stripe to GND)
  - [ ] Connect SIM800L TXD -> Arduino **Digital Pin 10**
  - [ ] Connect SIM800L RXD -> Arduino **Digital Pin 11**
  - [ ] Connect SIM800L RST -> Arduino **Digital Pin 12**
  - [ ] Leave SIM800L VDD pin **UNCONNECTED**
  - [ ] Insert active SIM card into module slot

- [ ] **Host PC & Arduino USB Connection**
  - [ ] Connect Arduino Uno to laptop via USB cable
  - [ ] Confirm green power LED on Arduino Uno is lit
  - [ ] Check Device Manager -> Ports (COM & LPT) to identify COM port number (e.g. `COM3`)

---

## Phase 2 — Software Environment & Database Setup

- [ ] **XAMPP Environment**
  - [ ] Launch XAMPP Control Panel
  - [ ] Start **Apache** service (Status: Green, Port: 80)
  - [ ] Start **MySQL** service (Status: Green, Port: 3306)

- [ ] **MySQL Database Initialization**
  - [ ] Open browser to `http://localhost/phpmyadmin`
  - [ ] Navigate to the **Import** tab
  - [ ] Select `C:\xampp\htdocs\ABMDMS\database\database.sql`
  - [ ] Click **Import** and confirm successful schema creation
  - [ ] Verify database `motion_monitoring` contains tables `motion_logs` and `sms_logs`

- [ ] **Configuration Check (`config.php`)**
  - [ ] Confirm database credentials match local XAMPP settings
  - [ ] Set `date_default_timezone_set('Asia/Manila')` (or local timezone)
  - [ ] Confirm `ALLOWED_ZONES` contains `['ROOMA', 'ROOMB', 'ROOMC']`
  - [ ] Confirm `ALLOWED_EVENT_TYPES` contains `['MOTION_DETECTED', 'MOTION_STOPPED']`

---

## Phase 3 — Arduino Firmware Flashing & Serial Monitor Verification

- [ ] Launch Arduino IDE
- [ ] Open project sketch `arduino/motion_sensor/motion_sensor.ino`
- [ ] Select **Tools > Board > Arduino Uno**
- [ ] Select **Tools > Port > [Your COM Port]**
- [ ] Verify recipient phone number in sketch (line 125): `const char SMS_RECIPIENT[] = "+63...";`
- [ ] Click **Upload** and wait for "Done uploading"
- [ ] Open **Tools > Serial Monitor** and set baud rate to **9600**
- [ ] Wait 30 seconds for PIR warm-up countdown (`[SYSTEM] Warm-up complete. System Ready.`)
- [ ] Wave hand in front of Sensor 1 (Pin 2) -> Verify `ROOMC_MOTION_DETECTED` prints ONCE
- [ ] Hold still -> Verify `ROOMC_MOTION_STOPPED` prints ONCE
- [ ] Repeat test for Sensor 2 (Pin 3 `ROOMA`) and Sensor 3 (Pin 4 `ROOMB`)

---

## Phase 4 — Serial Bridge Listener Activation

- [ ] Open PowerShell command prompt as Administrator
- [ ] Change directory: `cd C:\xampp\htdocs\ABMDMS\serial`
- [ ] List active COM ports: `.\list_ports.bat`
- [ ] Start listener for active port (replace COM3 with actual port):
  ```powershell
  .\start_reader.bat COM3
  ```
- [ ] Confirm reader output displays `[STATUS] Listening on COM3 at 9600 baud...`

---

## Phase 5 — API & Web Dashboard Integration Test

- [ ] Open Web Dashboard in browser: `http://localhost/ABMDMS/`
- [ ] Verify top navbar shows green badge: **System Status: Live**
- [ ] Verify 4 Summary Cards display:
  - Current Status (Clear / No Motion)
  - Total Motion Events
  - Today's Events
  - Last Motion Timestamp
- [ ] Open API Simulator Tool: `http://localhost/ABMDMS/tools/simulate_motion.php`
- [ ] Click **Check API Connection** -> Confirm output reads `API OK`
- [ ] Click **Simulate MOTION_DETECTED (ROOMA)**
- [ ] Check Dashboard -> Confirm visual alert changes to red pulsing **MOTION DETECTED** in ~3s
- [ ] Click **Simulate MOTION_STOPPED (ROOMA)** -> Confirm status returns to green **NO MOTION**

---

## Phase 6 — End-to-End Hardware Integration Test

- [ ] Wave hand in front of Room A PIR sensor (Pin 3)
- [ ] Observe Arduino built-in LED (Pin 13) lights up
- [ ] Check Serial Reader output window -> Confirm POST request sent to `api/record_motion.php` with `HTTP 200 OK`
- [ ] Check Dashboard (`http://localhost/ABMDMS/`) -> Confirm Room A triggers live motion alert
- [ ] Check target mobile phone -> Confirm SMS alert received: `[ABMDMS] ALERT: Motion detected in Room A!`
- [ ] Check phpMyAdmin -> `sms_logs` table -> Confirm row inserted with status `SENT`
- [ ] Trigger motion again in Room A within 60 seconds -> Confirm Serial Reader logs `SMS_SKIP:ROOMA:COOLDOWN` (cooldown protection active)

---

## Phase 7 — Android Mobile App API Integration (Optional)

- [ ] Test JSON REST Endpoint:
  `GET http://localhost/ABMDMS/api/get_motion_logs.php?limit=10`
- [ ] Verify JSON structure contains `success: true`, `total_events`, `latest_event`, and `events` array
- [ ] Test Zone Filter Endpoint:
  `GET http://localhost/ABMDMS/api/get_motion_logs.php?zone=ROOMA`
- [ ] Integrate endpoint into Android mobile app HTTP client (Retrofit / Volley)

---

## Phase 8 — System Sign-Off

| Milestone | Verified By | Status | Date |
|---|---|---|---|
| Hardware Wiring & Power | Hardware Lead | [ ] Approved | YYYY-MM-DD |
| Firmware & Serial Bridge | Embedded Systems | [ ] Approved | YYYY-MM-DD |
| Web Backend & Database | Software Lead | [ ] Approved | YYYY-MM-DD |
| SMS GSM Alert Dispatch | System Admin | [ ] Approved | YYYY-MM-DD |
| End-to-End Deployment | Project Manager | [ ] Approved | YYYY-MM-DD |
