# ABMDMS — Arduino Based Motion Detection Monitoring System

A beginner-friendly motion detection system. An **Arduino Uno** with an **HC-SR501 PIR sensor** detects movement, sends it to your laptop through the **USB cable**, and a **PHP + MySQL** web dashboard records and displays every event in real time.

```
Arduino Uno  →  PIR Sensor  →  USB Serial  →  Serial Reader  →  PHP API  →  MySQL  →  Web Dashboard
```

---

## 1. Project Overview

When someone walks in front of the PIR sensor, the Arduino prints `MOTION_DETECTED` over the USB cable. A small PHP program on your laptop (the "serial reader") is listening to that cable. It catches the message and sends it to a PHP web API, which saves it into a MySQL database. The dashboard in your browser then updates by itself — no page refresh needed.

---

## 2. Features

- Detects motion with an HC-SR501 PIR sensor
- Sends events over USB serial to the laptop
- Saves every event into a MySQL database
- Live web dashboard that updates every 3 seconds automatically
- Four summary cards: Current Status, Total Events, Today's Events, Last Motion
- Full motion history table with pagination
- Big colour-coded status indicator (green = clear, red pulsing = motion)
- Duplicate protection in **both** the Arduino code and the serial reader
- Built-in **Test Tool** so you can demo the system even without the hardware
- Secure: prepared SQL statements, whitelist validation, no credentials in the browser

---

## 3. Hardware Requirements

| Item | Notes |
|---|---|
| Arduino Uno | Any Uno or compatible clone |
| HC-SR501 PIR Motion Sensor | The common 3-pin white-dome sensor |
| Breadboard | Optional but easier |
| Jumper wires | 3 female-to-male wires |
| USB cable | The printer-style cable that came with the Uno |
| Windows laptop | This guide is written for Windows |

---

## 4. Software Requirements

| Software | Why | Already installed? |
|---|---|---|
| **XAMPP** (Apache + MySQL + PHP 8) | Runs the website and database | Yes — `C:\xampp` |
| **Arduino IDE** | Uploads the code to the Arduino | Download from arduino.cc |
| Python | **NOT needed.** The serial reader is written in PHP. | — |

---

## 5. Hardware Wiring

The HC-SR501 has **3 pins** underneath the white dome. Lift or look under the dome — the labels are printed on the board.

| PIR Sensor Pin | Connects To | Wire colour suggestion |
|---|---|---|
| **VCC** | Arduino **5V** | Red |
| **GND** | Arduino **GND** | Black |
| **OUT** | Arduino **Digital Pin 2** | Yellow |

### How to identify the pins

Most HC-SR501 boards print the labels next to the 3 metal pins. The usual order is `VCC — OUT — GND`, but **some boards are `GND — OUT — VCC`**, so always read the printed labels rather than assuming. Getting VCC and GND backwards will make the sensor get warm and it will not work.

### Connecting the Arduino to the laptop

Plug the square end of the USB cable into the Arduino and the flat end into your laptop. The green **ON** LED on the Arduino should light up. Windows will install the driver automatically the first time.

### ⚠️ PIR warm-up

The HC-SR501 needs **about 30–60 seconds after power on** to calibrate itself. During this time it gives false readings. The Arduino sketch includes a 30-second countdown for this reason. **Stay still and out of range while it counts down.**

### Sensitivity and delay knobs

There are two small orange screws on the sensor:

- **Sensitivity** — how far it can see (3 to 7 metres)
- **Time delay** — how long the output stays HIGH after motion (5 seconds to 5 minutes)

Turn the **time delay** knob fully **anti-clockwise** for the shortest delay. This makes the demo more responsive.

There is also a small yellow **jumper** with two positions:

- **H** (repeat trigger) — the output stays HIGH while movement continues. **Use this one.**
- **L** (single trigger) — the output pulses on and off.

---

## 6. Arduino Setup

1. Open the **Arduino IDE**.
2. Open the file `arduino\motion_sensor\motion_sensor.ino` from this project.
3. Go to **Tools → Board → Arduino Uno**.
4. Go to **Tools → Port** and select the COM port (e.g. `COM3`). **Write this number down** — you will need it later.
5. Click the **Upload** arrow (→).
6. Wait for "Done uploading".
7. Open **Tools → Serial Monitor**.
8. Set the baud rate at the bottom right to **9600**.

You should see:

```
ABMDMS - Motion Detection System
Warming up PIR sensor, please stay still (30 seconds)...
30...
29...
...
System Ready
Waiting for motion...
```

Now wave your hand in front of the sensor:

```
MOTION_DETECTED
MOTION_STOPPED
```

The orange LED labelled **L** on the Arduino board also lights up when motion is detected.

---

## 7. XAMPP Setup

1. Open the **XAMPP Control Panel**.
2. Click **Start** next to **Apache**.
3. Click **Start** next to **MySQL**.
4. Both should turn green.
5. Test it: open `http://localhost` in your browser.

---

## 8. Database Setup

1. Go to `http://localhost/phpmyadmin`
2. Click the **Import** tab at the top.
3. Click **Choose File** and select `C:\xampp\htdocs\ABMDMS\database\database.sql`
4. Scroll down and click **Import**.

You do **not** need to create the database by hand — the file does it all. When it finishes you will see a database called `motion_monitoring` on the left with a table called `motion_logs` inside it.

---

## 9. PHP Setup

The project must live at exactly this path:

```
C:\xampp\htdocs\ABMDMS\
```

If your MySQL has a password (the XAMPP default has none), open `config.php` and change this line:

```php
define('DB_PASS', '');   // put your password between the quotes
```

You can also change your time zone in the same file:

```php
date_default_timezone_set('Asia/Manila');
```

Now open the dashboard:

```
http://localhost/ABMDMS/
```

**Test it without the Arduino:** open `http://localhost/ABMDMS/tools/simulate_motion.php`, open the dashboard in a second browser tab, and click **Simulate MOTION_DETECTED**. The dashboard should turn red within 3 seconds all by itself.

---

## 10. Serial Reader Setup

The serial reader is the bridge between the Arduino and the website. It is written in PHP, so **nothing extra needs to be installed**.

### Step 1 — Find your COM port

Double-click:

```
serial\list_ports.bat
```

It lists every COM port on your computer. If you are not sure which one is the Arduino, run it once with the Arduino unplugged, then plug it in and run it again — the port that appeared the second time is your Arduino. You can also see it in Arduino IDE under **Tools → Port**.

### Step 2 — Put the port into the settings

Open `serial\serial_reader.php` in Notepad and edit the settings block near the top:

```php
$COM_PORT  = 'COM3';   // <-- change this to YOUR port
$BAUD_RATE = 9600;
$API_URL   = 'http://localhost/ABMDMS/api/record_motion.php';
```

### Step 3 — Close the Serial Monitor ⚠️

**This is the most common mistake.** Only one program can use a COM port at a time. If the Arduino IDE Serial Monitor is open, the serial reader cannot connect. Close it.

### Step 4 — Run it

Double-click:

```
serial\start_reader.bat
```

You should see:

```
----------------------------------------------------------
  ABMDMS - Arduino Serial Reader (PHP bridge)
----------------------------------------------------------
  COM port : COM3
  Baud rate: 9600
  API URL  : http://localhost/ABMDMS/api/record_motion.php
----------------------------------------------------------

[01:15:22] Connected to COM3. Waiting for motion events...

    Arduino: System Ready
    Arduino: Waiting for motion...
    Arduino: MOTION_DETECTED
[01:15:48] [OK]   MOTION_DETECTED -> saved as record #1
```

Leave this black window **open** the whole time the system is running. Press **Ctrl + C** to stop it.

> **Note for COM10 and above:** Windows needs a special name for ports numbered 10 or higher. The script handles this for you automatically — just type `COM12` normally in the settings.

---

## 11. How to Run the System

Every time you want to demo the project, do these five things in order:

1. **Start XAMPP** → Apache **and** MySQL (both green)
2. **Plug in the Arduino** (wait about 30 seconds for the PIR warm-up)
3. **Close the Arduino IDE Serial Monitor** if it is open
4. **Double-click `serial\start_reader.bat`** and leave the window open
5. **Open** `http://localhost/ABMDMS/` in your browser

Now walk in front of the sensor. Within about 3 seconds the dashboard turns **red**, the counters go up, and a new row appears at the top of the history table.

---

## 12. Troubleshooting

### Arduino not detected

- Try a different USB cable — many cheap cables are **charge-only** and carry no data
- Check **Device Manager → Ports (COM & LPT)** for your Arduino
- If it shows a yellow warning triangle, the driver is missing. Clone boards usually need the **CH340** driver
- In Arduino IDE check **Tools → Port** is not greyed out
- Try a different USB socket on the laptop

### PIR not detecting motion

- **Did you wait for the warm-up?** It needs 30–60 seconds of stillness after power on
- Double-check **VCC → 5V** and **GND → GND**. Backwards wiring stops it working
- Check **OUT → Digital Pin 2** (not pin 3, not an analog pin)
- Turn the **sensitivity** screw clockwise to increase the range
- Turn the **time delay** screw fully anti-clockwise for a faster response
- Set the yellow jumper to the **H** position
- PIR sensors detect *body heat in motion*. Waving a piece of paper will not trigger it — use your hand

### PHP cannot connect to MySQL

- Is **MySQL green** in the XAMPP Control Panel? If it will not start, something else is using port 3306
- Did you import `database.sql`? Check phpMyAdmin for the `motion_monitoring` database
- Check the username in `config.php` is `root` and the password is empty
- Check the port is `3306`

### Serial reader cannot connect

- **Close the Arduino IDE Serial Monitor** — this is the #1 cause
- Check `$COM_PORT` in `serial_reader.php` matches the real port (run `list_ports.bat`)
- Check `$BAUD_RATE` is `9600` — it must match `Serial.begin(9600)` in the sketch
- Make sure only one copy of `start_reader.bat` is running
- Unplug and replug the Arduino, then start the reader again

### Motion detected but not saved

- Is the black serial reader window still open and running?
- Does it say `[OK]` or `[FAIL]`? The `[FAIL]` message tells you what went wrong
- Is **Apache** running in XAMPP?
- Is **MySQL** running in XAMPP?
- Test the API by itself with the **Test Tool** page — if that fails too, the problem is PHP/MySQL, not the Arduino
- Check the Apache error log: `C:\xampp\apache\logs\error.log`

### Dashboard not updating

- Look at the badge in the top-right. If it says **Offline**, the page cannot reach the server
- Press **F12** in your browser and look at the **Console** tab for red errors
- Check you are using the URL `http://localhost/ABMDMS/` and **not** opening `index.php` as a file from the folder
- Open `http://localhost/ABMDMS/api/get_motion_logs.php` directly — you should see JSON text
- Make sure Apache is running

---

## 13. Project Structure

```
C:\xampp\htdocs\ABMDMS\
│
├── index.php                  The dashboard page
├── config.php                 Settings: database, time zone, event types
├── database.php               Reusable MySQL connection (PDO)
│
├── api\
│   ├── record_motion.php      Saves one motion event  (POST)
│   └── get_motion_logs.php    Returns stats + history (GET, JSON)
│
├── serial\
│   ├── serial_reader.php      The USB bridge program
│   ├── start_reader.bat       Double-click to start the bridge
│   └── list_ports.bat         Double-click to find your COM port
│
├── tools\
│   └── simulate_motion.php    Test page — fake motion without hardware
│
├── assets\
│   ├── css\style.css          All the styling
│   └── js\dashboard.js        Live auto-updating logic
│
├── database\
│   └── database.sql           Creates the database and table
│
├── arduino\motion_sensor\
│   └── motion_sensor.ino      The Arduino program
│
├── README.md                  This file
└── CHECKLIST.md               Step-by-step build checklist
```

### How the pieces talk to each other

| From | To | How |
|---|---|---|
| `motion_sensor.ino` | `serial_reader.php` | USB serial text at 9600 baud |
| `serial_reader.php` | `api/record_motion.php` | HTTP POST |
| `api/record_motion.php` | MySQL `motion_logs` | PDO prepared statement |
| `dashboard.js` | `api/get_motion_logs.php` | `fetch()` every 3 seconds |

---

## Security notes

This is a local school/demo project, so security is kept appropriate for the scope:

- All database queries use **prepared statements** — SQL injection is not possible
- `event_type` is checked against a **whitelist** in `config.php`; anything else is rejected
- Database credentials live only in `config.php` (server side) and are **never** sent to the browser
- API errors return a generic message; the real reason goes to the Apache error log
- All data is escaped before being placed into the HTML table

---

## Quick reference

| What | Where |
|---|---|
| Dashboard | `http://localhost/ABMDMS/` |
| Test Tool | `http://localhost/ABMDMS/tools/simulate_motion.php` |
| Record API | `http://localhost/ABMDMS/api/record_motion.php` |
| Logs API | `http://localhost/ABMDMS/api/get_motion_logs.php` |
| phpMyAdmin | `http://localhost/phpmyadmin` |
| Database | `motion_monitoring` → table `motion_logs` |
| Baud rate | `9600` |
| PIR pin | Digital Pin **2** |
