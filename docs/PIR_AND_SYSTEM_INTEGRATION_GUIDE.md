# ABMDMS — PIR Sensor & Complete System Integration Guide

## 1. Executive Summary & System Overview

The **Arduino-Based Motion Detection & Monitoring System (ABMDMS)** is a multi-zone intrusion detection, real-time web monitoring, and automated cellular SMS alerting solution. 

This guide details the end-to-end implementation and integration of:
1. **Multi-Zone HC-SR501 PIR Motion Sensors** for hardware intrusion detection.
2. **SIM800L GSM SMS Module** for immediate SMS alert dispatching directly to mobile phones.
3. **USB Serial Reader Bridge** (PowerShell / PHP) for relaying hardware events to the web backend.
4. **PHP & MySQL REST Web Backend** for logging, event state tracking, and data storage.
5. **Real-Time Live Web Dashboard** featuring multi-zone status cards, event history, pagination, and SMS logs.
6. **Mobile App & API Integration** endpoints for remote mobile monitoring.

---

## 2. Architecture & Data Flow

```
+------------------+         +-------------------+         +---------------------+
| HC-SR501 PIR     | ------->| Arduino Uno       | ------->| SIM800L GSM Module  | ---> SMS Alert
| Motion Sensors   | (Digital| Microcontroller   | (Software| (Cellular Network)  |     to Phone
| (Pins 2, 3, 4)   |  Pins)  |                   |  Serial)|                     |
+------------------+         +---------+---------+         +---------------------+
                                       |
                                  (USB Serial)
                                       v
                             +-------------------+
                             | Serial Reader     |
                             | (PowerShell / PHP)|
                             +---------+---------+
                                       |
                                  (HTTP POST)
                                       v
                             +-------------------+
                             | PHP Web API       |
                             | (XAMPP Backend)   |
                             +---------+---------+
                                       |
                             +---------+---------+
                             |                   |
                             v                   v
                     +---------------+   +---------------+
                     | MySQL Database|   | Web Dashboard |
                     | (motion_logs) |   | (index.php)   |
                     +---------------+   +---------------+
```

---

## 3. Hardware Requirements & Wiring Blueprint

### 3.1 Bill of Materials
| Component | Function | Operating Specifications |
|---|---|---|
| **Arduino Uno R3** | Main Microcontroller Unit | 5V Logic, USB Serial Interface (5V pin left unconnected) |
| **HW-131 Power Module** | Breadboard Power Supply (MB-102 compatible) | Sets dual rails to 5V; master ON/OFF switch |
| **12V DC Wall Adapter** | External Power Source | 12V DC, 1A to 2A (5.5mm x 2.1mm barrel jack) |
| **HC-SR501 PIR Sensor (3x)** | Passive Infrared Motion Detection | 5V VCC, 3.3V OUT Signal, 120° Detection Angle |
| **5V Piezoelectric Buzzer** | Local Audible Intrusion Alarm | 5V Active Piezo Buzzer (~20mA draw) |
| **SIM800L V2.2 GSM Module** | SMS Alert Transmitter | 5V V2.2 Board, 2A Peak Current, Micro-SIM card |
| **1000µF Electrolytic Capacitor** | Power Smoothing for GSM Transmit Bursts | Rated >= 16V (Observe Polarity!) |
| **Solderless Breadboard & Jumper Wires** | Circuit Interconnects | Male-to-Male, Female-to-Male Jumper Wires |

---

### 3.2 PIR Motion Sensor Pinout & Configuration

Each **HC-SR501 PIR Sensor** has three bottom pins and two adjustment potentiometers under the dome:

```
        +----------------------------+
        |     HC-SR501 Top Dome      |
        +----------------------------+
         [ Potentiometer 1 ] [ Potentiometer 2 ]
          (Sensitivity)       (Time Delay)
        ------------------------------
           [ VCC ]   [ OUT ]   [ GND ]
```

#### Sensor Potentiometer & Jumper Setup:
* **Trigger Mode Jumper**: Set to **H** (Repeatable Trigger / Retriggerable mode).
* **Sensitivity Adjustment**: Clockwise increases range (~7 meters); counter-clockwise decreases (~3 meters). Set to mid-point.
* **Time Delay Adjustment**: Turn **fully counter-clockwise** for the shortest hold delay (~3 to 5 seconds).

#### Multi-Zone & Alarm Pin Assignment:
| Arduino Pin | Zone Code / Device | Friendly Name | Hardware Connection |
|---|---|---|---|
| **Digital Pin 2** | `ROOMC` | Room C | PIR 1 Signal OUT |
| **Digital Pin 3** | `ROOMA` | Room A | PIR 2 Signal OUT |
| **Digital Pin 4** | `ROOMB` | Room B | PIR 3 Signal OUT |
| **Digital Pin 5** | `ROOMD` | Room D (Optional) | PIR 4 Signal OUT (See section 8 for enabling) |
| **Digital Pin 8** | `BUZZER` | 5V Piezo Buzzer | Positive (+) leg (longer pin) |
| **5V Rail** | Common VCC | All Sensors VCC | Breadboard Bottom Power Rail (+) from HW-131 |
| **GND Rail** | Common GND | All Sensors & Buzzer GND | Breadboard Ground Rail (-) |

---

### 3.3 SIM800L GSM Module & Power Supply Wiring

> [!CRITICAL]
> **POWER ARCHITECTURE**:
> * Plug the **12V DC wall adapter** into the **HW-131 Breadboard Power Module**.
> * Set both the top and bottom HW-131 jumper blocks to **5V**.
> * Connect the **Arduino 5V pin to the HW-131 5V rail (+)** to supply 5V operating power to the Arduino Uno.
> * Connect **Arduino GND** to the breadboard ground rail for common 0V logic ground.
> * Connect the **USB cable to the PC/laptop strictly for serial data communication** with the backend listener.
> * Place a **1000µF (>=16V) capacitor in parallel** directly across SIM800L `5Vin` and `GND`.

| SIM800L V2.2 Pin | Connects To | Notes |
|---|---|---|
| **5Vin** | HW-131 Top 5V Rail (+) | Connect 1000µF capacitor (+) leg |
| **GND** | Breadboard Ground Rail (-) | Common ground with Arduino and HW-131 |
| **TXD** | Arduino Digital Pin 10 | SoftwareSerial RX |
| **RXD** | Arduino Digital Pin 11 | SoftwareSerial TX (Direct for V2.2 5V board) |
| **RST** | Arduino Digital Pin 12 | Module reset control line |
| **VDD** | *Unconnected* | Output reference pin (Do NOT feed 5V!) |

---

## 4. Software & System Configuration

### 4.1 XAMPP & Database Installation
1. Start **Apache** and **MySQL** in the XAMPP Control Panel.
2. Ensure project files are situated at `C:\xampp\htdocs\ABMDMS\`.
3. Open phpMyAdmin at `http://localhost/phpmyadmin`.
4. Import `database/database.sql` to instantiate the database schema:
   - Database: `motion_monitoring`
   - Tables: `motion_logs` and `sms_logs`

### 4.2 Application Configuration (`config.php`)
Verify database credentials and zone definitions in `config.php`:

```php
define('DB_HOST', 'localhost');
define('DB_NAME', 'motion_monitoring');
define('DB_USER', 'root');
define('DB_PASS', '');

date_default_timezone_set('Asia/Manila');

define('ALLOWED_EVENT_TYPES', ['MOTION_DETECTED', 'MOTION_STOPPED']);
define('ALLOWED_ZONES', ['ROOMA', 'ROOMB', 'ROOMC']);
define('ZONE_LABELS', [
    'ROOMA' => 'Room A',
    'ROOMB' => 'Room B',
    'ROOMC' => 'Room C',
]);
define('SMS_RECIPIENT_DISPLAY', '+639169751409');
```

---

## 5. Arduino Firmware Deployment

1. Connect the Arduino Uno to your PC via USB cable.
2. Launch **Arduino IDE** and open `arduino/motion_sensor/motion_sensor.ino`.
3. Select Board: `Tools > Board > Arduino Uno`.
4. Select Port: `Tools > Port > COMx` (e.g., `COM3` or `COM4`).
5. Update recipient phone number in line 125 of `motion_sensor.ino` if needed:
   ```cpp
   const char SMS_RECIPIENT[] = "+639169751409";
   ```
6. Click **Upload**.
7. Open **Serial Monitor** (Baud rate `9600`).
8. Wait 30 seconds for the PIR warm-up sequence to complete (`System Ready`).

---

## 6. Serial Reader Bridge Setup

The Serial Reader bridge listens to serial messages printed by the Arduino over USB and forwards them via HTTP POST to the PHP backend.

### Running PowerShell Listener:
1. Open PowerShell as Administrator.
2. Navigate to project serial folder:
   ```powershell
   cd C:\xampp\htdocs\ABMDMS\serial
   ```
3. Run the port discovery script:
   ```powershell
   .\list_ports.bat
   ```
4. Start the listener for your Arduino COM port (e.g., COM3):
   ```powershell
   .\start_reader.bat COM3
   ```

---

## 7. Web Dashboard & API Verification

### 7.1 Real-Time Web Dashboard
* Open browser: `http://localhost/ABMDMS/`
* Verify status indicator reads **NO MOTION** (Green).
* Verify status cards count total events, today's events, and recent SMS dispatches.
* Auto-refresh polls every 3 seconds seamlessly without page reload.

### 7.2 Simulation & Hardware Independent Testing
* Open simulator tool: `http://localhost/ABMDMS/tools/simulate_motion.php`
* Click **Check API Connection** to verify database connectivity.
* Click **Simulate MOTION_DETECTED** for `ROOMA`, `ROOMB`, or `ROOMC`.
* Confirm dashboard updates to **MOTION DETECTED** (Red pulse) immediately.
* Click **Simulate MOTION_STOPPED** to verify clear state transition.

---

## 8. Restoring / Expanding Room D (Pin 5 Integration)

To enable the 4th zone (**Room D** on Pin 5):

1. **Hardware**: Connect PIR Sensor 4 OUT signal wire to **Arduino Pin 5**.
2. **`config.php`**: Add `'ROOMD'` to `ALLOWED_ZONES` and `'ROOMD' => 'Room D'` to `ZONE_LABELS`.
3. **`motion_sensor.ino`**: Change `NUM_ZONES = 4` and update arrays:
   ```cpp
   const int   PIR_PIN[NUM_ZONES]   = {  2,       3,       4,       5      };
   const char* ZONE_NAME[NUM_ZONES] = { "ROOMC", "ROOMA", "ROOMB", "ROOMD" };
   const char* ZONE_TEXT[NUM_ZONES] = { "Room C","Room A","Room B","Room D" };
   ```
4. **Serial Readers**: Update regex in `serial/serial_reader.ps1` and `serial/serial_reader.php` to include `ROOMD`.

---

## 9. Troubleshooting & Maintenance Matrix

| Symptom / Error | Probable Cause | Corrective Action |
|---|---|---|
| Continuous false PIR triggers | PIR warming up or time delay set too long | Wait 30s after boot; turn time-delay potentiometer fully counter-clockwise; set jumper to **H**. |
| Arduino continuously resets when SMS sends | SIM800L power supply current limit exceeded | Use external 5V 2A power supply; add 1000µF capacitor across SIM800L power pins; share common GND. |
| Dashboard shows "Live (Disconnected)" | Serial reader script not running or wrong COM port | Run `serial/start_reader.bat COMx` with the correct COM port. |
| API returns `400 Invalid Zone` | Zone string not in `ALLOWED_ZONES` array | Check `config.php` definitions matches Arduino zone names (`ROOMA`, `ROOMB`, `ROOMC`). |
| Database connection error | XAMPP MySQL down or wrong port | Verify MySQL service is running in XAMPP panel on port 3306. |
