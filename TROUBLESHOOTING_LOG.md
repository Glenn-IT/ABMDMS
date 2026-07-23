# Troubleshooting Log — Things I Hit & How I Fixed Them

A personal reference from building **ABMDMS** (Arduino PIR + XAMPP + PHP + MySQL).
Written so it also helps on **future Arduino / web projects**.

Format for each problem: **Symptom → Why it happens → Fix**.

---

## 1. "Sketch uses 2802 bytes (8%)..." — is it done? Do I wait?

**Symptom:** After clicking a button in Arduino IDE, it shows a memory usage message and seems to stop.

**Why:** That message just means the code **compiled with no errors**. It does *not* mean the code is on the board yet. It depends which button you pressed:
- **Checkmark (✔ Verify)** = only checks the code. Nothing was uploaded.
- **Arrow (→ Upload)** = checks *and* sends it to the board.

**Fix:**
- If it stopped after the memory message → you clicked Verify. Click the **→ arrow** to actually upload.
- Wait for **"Done uploading."** at the bottom. That's the real finish line.
- Low percentages (8%, 16%) are good — plenty of room. Nothing to worry about.

> Rule of thumb: **"Done uploading" = success.** The memory message alone is not.

---

## 2. PIR sensor reports motion when I'm NOT moving

**Symptom:** `MOTION_DETECTED` / `MOTION_STOPPED` print randomly while I sit still.

**Why (most common):** The HC-SR501 needs a **30–60 second warm-up** after power-on, and it only calibrates if **nothing moves near it** during that time. People (me!) hover over it watching, which keeps re-triggering it.

**Fix:**
1. Reset the Arduino (or unplug/replug).
2. **Walk away** — don't lean over the sensor.
3. Let it finish the warm-up countdown + stay still ~20 more seconds.
4. Then test. If it's quiet when still and only fires when you move → calibrated.

**Other causes if warm-up doesn't fix it:**
- **Sensitivity knob too high** → turn the orange screw slightly anti-clockwise.
- **Environmental heat in motion** → point it away from windows, fans, aircon, and the laptop.
- Yellow jumper should be on **H** (repeat trigger), not **L**.

---

## 3. Two lights blink on the Arduino during motion — is that a bug?

**Symptom:** An orange **TX** light flashes, and the **L** light turns on/off.

**Why:** This is **normal and correct**, not a bug:
- **TX** = "transmit" — flashes whenever the Arduino sends serial data (our motion message).
- **L** = the built-in LED on Pin 13 — our code turns it ON during motion as a visual cue.

**Fix:** Nothing. Seeing these means it's working. ✅

---

## 4. PIR STILL detects even when I cover the sensor

**Symptom:** Motion fires no matter what — even with the dome covered.

**Why:** The **OUT signal isn't reaching the Arduino pin**, so the input pin "floats" and reads random HIGH/LOW noise. Usually a **loose wire, wrong hole, or wrong sensor pin**. On the HC-SR501 the pin order (VCC / OUT / GND) is **printed under the dome and differs between boards** — don't assume the order.

**Fix:**
1. Push all 3 wires in firmly (wobbly breadboard = floating pin).
2. **Read the printed labels under the dome** and make sure OUT→Pin 2, VCC→5V, GND→GND. (This was my exact bug — a swapped wire.)
3. Isolation test: pull OUT off Pin 2 and jumper **Pin 2 → GND**. If it goes quiet, the Arduino/code is fine and the problem is the sensor wiring.

> Lesson: "works no matter what I do" almost always = a floating/disconnected input, not a broken sensor.

---

## 5. "Could not open COM5. Another program may be using it." (Serial Monitor is closed!)

**Symptom:** The serial bridge can't open the port even though the Arduino IDE Serial Monitor tab is closed.

**Why:** On Windows, **only one program can hold a COM port at a time**, and:
- **Arduino IDE 2.x keeps the port open in the background** even after you close the Serial Monitor *tab*. Closing the tab is not enough.
- A previous reader window or a crashed session can leave the port **locked at the driver level** until the USB device is reset.

**Fix (in order):**
1. **Close the ENTIRE Arduino IDE** — the whole program, not just the monitor tab.
2. Close any old reader/terminal windows.
3. **Unplug the USB cable, wait 10 seconds, replug** — this force-releases a stuck port.
4. Still stuck? **Device Manager → Ports (COM & LPT) → right-click the port → Disable, wait 3s, Enable.**

> You don't need the Arduino IDE open to run the project — the code already lives on the board.

---

## 6. The COM port opens in some tools but PHP can't open it

**Symptom:** `mode COM5:` works, other tools see the port, but the PHP reader (`fopen('COM5:', ...)`) always fails to open it.

**Why:** **PHP on Windows is unreliable at opening COM ports.** On this PC (PHP 8.2 ZTS), `fopen` on a serial port fails even when the port is completely free. It's a PHP limitation, not a wiring or lock problem.

**How I confirmed it:** Opened the same port with **PowerShell / .NET `System.IO.Ports.SerialPort`** — it opened instantly. So the port was fine; PHP was the weak link.

**Fix:** Use a **PowerShell serial bridge** instead of PHP (`serial/serial_reader.ps1`, launched by `start_reader.bat`). PowerShell is built into Windows (no install) and uses solid .NET serial code. Everything else — the PHP API, MySQL, dashboard — stayed the same. Only the bridge changed.

> Future projects: for serial-over-USB on Windows, reach for **PowerShell (.NET SerialPort)** or **Python (pyserial)** before PHP.

---

## General debugging lessons (keep these!)

- **"Done uploading" is the only proof the code is on the board.** Compile messages aren't.
- **Isolate the layer.** Test each stage on its own: sensor in Serial Monitor → API with the Test Tool page → database in phpMyAdmin → dashboard in the browser. When something breaks, you instantly know which layer.
- **"Works no matter what I do" = a floating input or disconnected wire**, not a dead component.
- **One program per COM port.** If a port won't open, something already holds it — close IDEs fully, or unplug/replug to reset.
- **When one tool can't do a job, test if another can.** PHP couldn't open the port; PowerShell could — that one test found the real cause in seconds.
- **Read the actual labels on hardware.** Pin order varies between boards; don't trust the tutorial's photo.
- **Change one thing at a time** and re-test, so you know what actually fixed it.

---

## Quick "it's not working" checklist for next time

1. Is the code actually uploaded? (saw "Done uploading"?)
2. Did I wait for any sensor warm-up, standing clear?
3. Are all wires firm and on the **labeled** pins?
4. Is anything else holding the COM port? (close IDE fully, unplug/replug)
5. Are XAMPP **Apache** and **MySQL** both green?
6. Test each layer separately — which one fails first?
7. If a tool can't talk to hardware, try a different tool to confirm where the fault is.
