# ESP32 Wireless Migration & Wiring Guide (ABMDMS)

This guide explains how to migrate ABMDMS from an **Arduino Uno (USB Wired)** to an **ESP32 (Wi-Fi Wireless)**.

---

## 1. Do We Need to Change the Android Studio Code?

### **Answer: NO!**
You **do not need to change any code** inside your Android Studio project.

**Why?**
- The Android app connects to the **PHP API** (`GET api/get_motion_logs.php` on your XAMPP server) to read data.
- The Android app never connected directly to the Arduino or serial cable.
- Because the PHP backend, MySQL database structure, and JSON output format remain **100% identical**, the Android app will continue working seamlessly as long as the Android phone and the PC running XAMPP are connected to the same Wi-Fi network.

---

## 2. Hardware Wiring Diagram (ESP32)

### A. Power Supply (HW-131 5V Breadboard Module)
* **HW-131 5V (+)** $\rightarrow$ Breadboard Red (+) Rail $\rightarrow$ **ESP32 VIN / 5V Pin**, **SIM800L 5Vin**, all **HC-SR501 PIR VCC**
* **HW-131 GND (-)** $\rightarrow$ Breadboard Blue (-) Rail $\rightarrow$ **ESP32 GND**, **SIM800L GND**, all **PIR GND**, **Buzzer (-)**
*(All GND lines must be connected together).*

---

### B. HC-SR501 PIR Motion Sensors
*Note: HC-SR501 sensors take 5V power, but their `OUT` pin outputs 3.3V logic, making them **100% safe and direct-plug** into ESP32 GPIOs!*

| Sensor | Wire | Connect to ESP32 Pin |
|---|---|---|
| **PIR 1 (Room C)** | `OUT` | **GPIO 13** |
| **PIR 2 (Room A)** | `OUT` | **GPIO 12** |
| **PIR 3 (Room B)** | `OUT` | **GPIO 14** |
| *(Optional Room D)* | `OUT` | **GPIO 27** |

---

### C. SIM800L V2.2 GSM Module (Hardware UART2)
The ESP32 has dedicated Hardware UARTs, providing far better reliability than Arduino SoftwareSerial.

| SIM800L Pin | Connect to ESP32 Pin | Notes |
|---|---|---|
| **TXD** | **GPIO 16 (RX2)** | Direct wire (or through 1k/2k divider) |
| **RXD** | **GPIO 17 (TX2)** | Direct wire (ESP32 3.3V is safe for SIM800L) |
| **RST** | **GPIO 4** | Active LOW reset |
| **5Vin** | Breadboard 5V Rail | High current (HW-131) |
| **GND** | Breadboard GND Rail | Common Ground |
| **1000µF Capacitor** | Across 5Vin & GND | Electrolytic capacitor for burst currents |

---

### D. MAX98357A I2S Audio Amplifier & Speaker
The ESP32 uses dedicated hardware I2S DMA channels to drive the MAX98357A 3W Class-D amplifier:

| MAX98357A Pin | Connect to ESP32 / Rail | Function / Notes |
|---|---|---|
| **LRC (WS)** | **GPIO 25** | I2S Word Select Clock |
| **BCLK** | **GPIO 26** | I2S Bit Clock |
| **DIN** | **GPIO 27** | I2S Digital Audio Data |
| **VIN** | Breadboard 5V Rail | Power supply (5V gives 3W loudness) |
| **GND** | Breadboard GND Rail | Common Ground with ESP32 |
| **GAIN** | *Unconnected* | Defaults to 9dB gain (connect to GND for 3dB) |
| **SD** | *Unconnected* | Default (Left + Right mix to mono) |
| **SPK + / -** | **Speaker (+/-)** | Direct connection to 4Ω or 8Ω speaker |

> **⚠️ CRITICAL SPEAKER NOTE (BTL Output):** The MAX98357A uses Bridge-Tied Load (BTL) drive. **NEVER connect SPK(-) or SPK(+) to Breadboard Ground or 5V!** Connect only to the two speaker terminals.
> 
> For interactive schematics and the 830-point hole-by-hole breadboard diagram, open [wiring.html](file:///C:/xampp/htdocs/ABMDMS/wiring.html).

---

## 3. Software Configuration

### Step 1: Install ESP32 Board in Arduino IDE
1. Open **Arduino IDE**.
2. Go to `File` > `Preferences`.
3. In **Additional Board Manager URLs**, paste:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
4. Go to `Tools` > `Board` > `Boards Manager...`, search for `esp32` by **Espressif Systems** and click **Install**.

### Step 2: Configure the Sketch
Open [arduino/esp32_motion_sensor/esp32_motion_sensor.ino](file:///C:/xampp/htdocs/ABMDMS/arduino/esp32_motion_sensor/esp32_motion_sensor.ino) and edit Section 1:

```cpp
// 1. Enter your Wi-Fi credentials
const char* WIFI_SSID     = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// 2. Enter your PC's Local IP Address (find it via `ipconfig` in Command Prompt)
const char* SERVER_IP     = "192.168.1.100";
```

### Step 3: Upload to ESP32
1. Select Board: `Tools` > `Board` > `esp32` > **ESP32 Dev Module** (or your specific ESP32 board).
2. Select COM Port: `Tools` > `Port`.
3. Click **Upload**.
4. Once flashed, open Serial Monitor at **115200 baud** to verify it connects to your Wi-Fi and logs events!

---

## 4. Retiring the Serial Bridge
Once the ESP32 is running:
- You **no longer need** `start_reader.bat` or `serial_reader.ps1`.
- The ESP32 sends HTTP POST requests directly to your XAMPP server over Wi-Fi!
