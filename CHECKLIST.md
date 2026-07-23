# ABMDMS — Build & Test Checklist

Tick each box as you go. Work through the phases **in order** — each one depends on the phase before it.

> Tip: in most Markdown editors you tick a box by changing `[ ]` to `[x]`.

---

## PHASE 1 — PREPARE HARDWARE

- [x] Prepare Arduino Uno
- [x] Prepare HC-SR501 PIR Sensor
- [x] Prepare breadboard
- [x] Prepare jumper wires
- [x] Prepare USB cable
- [x] Connect PIR **VCC** to Arduino **5V**
- [x] Connect PIR **GND** to Arduino **GND**
- [x] Connect PIR **OUT** to Arduino **Digital Pin 2**
- [x] Set the PIR yellow jumper to the **H** position
- [x] Turn the PIR time-delay screw fully anti-clockwise
- [x] Connect Arduino Uno to laptop
- [x] Confirm the Arduino green power LED is ON

---

## PHASE 2 — INSTALL SOFTWARE

- [x] Install Arduino IDE
- [x] Install XAMPP *(already installed at `C:\xampp`)*
- [x] Start Apache
- [x] Start MySQL
- [x] Confirm the Arduino is detected by the laptop (Device Manager → Ports)

> **Python is NOT needed.** The serial reader in this project is written in PHP, which XAMPP already provides.

---

## PHASE 3 — CONFIGURE ARDUINO

- [x] Open Arduino IDE
- [x] Open `arduino\motion_sensor\motion_sensor.ino`
- [x] Select **Tools → Board → Arduino Uno**
- [x] Select the correct COM port under **Tools → Port**
- [x] Write down the COM port number: `COM____`
- [x] Click Upload and wait for "Done uploading"
- [x] Open the Serial Monitor
- [x] Set the baud rate to **9600**
- [x] Wait for the 30-second PIR warm-up countdown to finish
- [x] Confirm `System Ready` appears
- [x] Wave your hand and confirm `MOTION_DETECTED` appears
- [x] Stay still and confirm `MOTION_STOPPED` appears
- [x] Confirm it does **not** print hundreds of repeated lines

---

## PHASE 4 — SET UP XAMPP

- [x] Open the XAMPP Control Panel
- [x] Start Apache (turns green)
- [x] Start MySQL (turns green)
- [x] Open `http://localhost/phpmyadmin`
- [x] Click the **Import** tab
- [x] Choose the file `database\database.sql`
- [x] Click **Import** and wait for the success message
- [x] Confirm the `motion_monitoring` database exists
- [x] Confirm the `motion_logs` table exists inside it
- [x] Click the table and confirm the 5 columns are there

---

## PHASE 5 — INSTALL PHP SYSTEM

- [x] Confirm the project is at `C:\xampp\htdocs\ABMDMS\`
- [x] Confirm the folder is named exactly `ABMDMS`
- [x] Open `config.php` and check the database settings
- [x] Set your time zone in `config.php` if needed
- [x] Open `http://localhost/ABMDMS/`
- [x] Confirm the dashboard loads with no red error box
- [x] Confirm the status shows **NO MOTION**
- [x] Confirm the badge in the top-right says **Live**

---

## PHASE 6 — TEST PHP API (no hardware needed)

- [x] Open `http://localhost/ABMDMS/tools/simulate_motion.php`
- [x] Click **Check API Connection** and confirm it says `API OK`
- [x] Open the dashboard in a **second browser tab**
- [x] Click **Simulate MOTION_DETECTED**
- [x] Confirm the black output box shows `SUCCESS`
- [x] Switch to the dashboard tab **without refreshing it**
- [x] Confirm the status turned **red — MOTION DETECTED** within ~3 seconds
- [x] Confirm Total Events and Today's Events went up by 1
- [x] Confirm a new row appeared at the top of the history table
- [x] Click **Simulate MOTION_STOPPED** and confirm the status turns green again
- [x] Open phpMyAdmin → `motion_logs` and confirm the rows are really saved

---

## PHASE 7 — CONFIGURE SERIAL BRIDGE

- [ ] Double-click `serial\list_ports.bat`
- [ ] Note which COM port is the Arduino
- [ ] Open `serial\serial_reader.php` in Notepad
- [ ] Set `$COM_PORT` to your port
- [ ] Confirm `$BAUD_RATE` is `9600`
- [ ] Confirm `$API_URL` is `http://localhost/ABMDMS/api/record_motion.php`
- [ ] Save the file
- [ ] **CLOSE the Arduino IDE Serial Monitor** ⚠️
- [ ] Double-click `serial\start_reader.bat`
- [ ] Confirm it says `Connected to COM__`
- [ ] Confirm you can see the Arduino's messages appearing in the window

---

## PHASE 8 — TEST FULL SYSTEM

- [ ] Start XAMPP Apache
- [ ] Start XAMPP MySQL
- [ ] Connect the Arduino
- [ ] Confirm the Arduino code is uploaded
- [ ] Close the Serial Monitor
- [ ] Start `start_reader.bat`
- [ ] Open the dashboard `http://localhost/ABMDMS/`
- [ ] Wait for the PIR warm-up to finish
- [ ] Walk in front of the PIR sensor
- [ ] Confirm the orange **L** LED on the Arduino lights up
- [ ] Confirm the serial reader window prints `[OK] MOTION_DETECTED`
- [ ] Confirm the dashboard turns **red** by itself
- [ ] Confirm the counters increase
- [ ] Confirm the new row appears in the history table with source `ARDUINO_PIR`
- [ ] Confirm the row exists in phpMyAdmin

---

## PHASE 9 — FINAL TESTING

- [ ] Test the no-motion state (dashboard stays green)
- [ ] Test a single motion detection
- [ ] Test repeated motion (walk back and forth)
- [ ] Test several motion events one after another
- [ ] **Check for duplicate records** — one walk-past should create one row, not fifty
- [ ] Test the pagination buttons on the history table
- [ ] Unplug the Arduino → confirm the reader says it is reconnecting
- [ ] Replug the Arduino → confirm the reader reconnects by itself
- [ ] Stop Apache → confirm the dashboard badge turns **Offline**
- [ ] Start Apache → confirm the badge returns to **Live**
- [ ] Stop MySQL → confirm you get a friendly error, not a crash
- [ ] Start MySQL → confirm everything recovers
- [ ] Refresh the dashboard and confirm the data is still there
- [ ] Resize the browser window and confirm the layout stays readable

---

## PHASE 10 — FINAL DOCUMENTATION

- [ ] Read through README.md
- [ ] Complete this CHECKLIST.md
- [ ] Document your wiring (take a close-up photo)
- [ ] Write down your COM port number somewhere safe
- [ ] Take a screenshot of the dashboard showing **NO MOTION**
- [ ] Take a screenshot of the dashboard showing **MOTION DETECTED**
- [ ] Take a screenshot of the serial reader window
- [ ] Take a screenshot of phpMyAdmin showing the records
- [ ] Take a photo of the full hardware setup
- [ ] Practise the demonstration from start to finish

---

## Before the demonstration — quick reminder

Do these five things in this order:

1. Start **Apache** and **MySQL** in XAMPP
2. Plug in the Arduino and wait ~30 seconds for the PIR warm-up
3. **Close** the Arduino IDE Serial Monitor
4. Double-click `serial\start_reader.bat` and leave the window open
5. Open `http://localhost/ABMDMS/`

**Backup plan:** if the hardware misbehaves during the demo, use
`http://localhost/ABMDMS/tools/simulate_motion.php` to show the dashboard working.
