# Wi-Fi & IP Configuration Guide (Network Migration Cheat-Sheet)

Whenever you switch to a new Wi-Fi network (home Wi-Fi, school/office Wi-Fi, or mobile phone hotspot), follow this quick 3-step checklist to update your IP address across the entire system.

---

## 🧭 Step 1: Find Your Laptop's New IP Address

1. Open **Command Prompt** (or **PowerShell**) on your laptop.
2. Type:
   ```cmd
   ipconfig
   ```
3. Look for your active network adapter (usually **Wireless LAN adapter Wi-Fi**).
4. Copy the **IPv4 Address** (e.g. `192.168.1.25` or `10.192.10.14`).

---

## 📱 Step 2: Update Android Studio (Mobile App)

Open file: [`app/src/main/java/.../data/ApiClient.kt`](file:///C:/Users/GLENN/AndroidStudioProjects/MBPSAASMobileBasedPoultrySecurityandAlertSystem/app/src/main/java/com/example/mbpsaas_mobile_based_poultry_security_and_alert_system/data/ApiClient.kt)

Change `BASE_URL` to your new IP address:

```kotlin
object ApiClient {

    // Change to your current Laptop Wi-Fi IP address:
    private const val BASE_URL = "http://YOUR_LAPTOP_IP/mbpsaas_api/"

    // Example:
    // private const val BASE_URL = "http://192.168.1.25/mbpsaas_api/"
```

👉 **Action**: In Android Studio, click **Run (Shift + F10)** to install the updated app to your phone.

---

## ⚡ Step 3: Update ESP32 Microcontroller (Arduino IDE)

Open file: [`arduino/esp32_motion_sensor/esp32_motion_sensor.ino`](file:///C:/xampp/htdocs/ABMDMS/arduino/esp32_motion_sensor/esp32_motion_sensor.ino)

Edit **SECTION 1** with your new Wi-Fi Name, Password, and Laptop IP:

```cpp
// ============================================================
// SECTION 1 - WI-FI & SERVER SETTINGS
// ============================================================

// 1. Enter your Wi-Fi name & password (must be 2.4GHz)
const char* WIFI_SSID     = "YOUR_NEW_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_NEW_WIFI_PASSWORD";

// 2. Enter your Laptop Local IPv4 Address
const char* SERVER_IP     = "YOUR_LAPTOP_IP";   // e.g. "192.168.1.25"
const int   SERVER_PORT   = 80;
```

👉 **Action**: Connect your ESP32 via USB and click **Upload (Ctrl + U)** in Arduino IDE.

---

## 🧪 Step 4: Quick 10-Second Test

To verify your setup is working before testing physical motion:

1. **Test XAMPP from your phone's browser**:
   Open Chrome on your phone and go to:
   ```text
   http://YOUR_LAPTOP_IP/mbpsaas_api/get_sensors.php
   ```
   *If you see JSON data `{"success":true,...}`, your phone can communicate with XAMPP!*

2. **Check ESP32 Serial Monitor**:
   Open Serial Monitor in Arduino IDE (**115200 baud**). You should see:
   ```text
   [Wi-Fi] Connected successfully!
   [Wi-Fi] ESP32 IP Address: 192.168.X.X
   [SENSOR CONTROL] Room A (ROOMA) is now ACTIVE
   ```

---

## ⚠️ Common Troubleshooting

| Issue | Cause & Solution |
|---|---|
| **"Server cannot reach 127..."** | Phone is looking at `localhost`. Update `ApiClient.kt` to the laptop's actual Wi-Fi IP address. |
| **Phone can't open XAMPP page** | 1. Ensure phone & laptop are on the exact same Wi-Fi.<br>2. In Windows Settings > Network, ensure your Wi-Fi is set to **Private Network** (not Public) so Windows Firewall allows port 80. |
| **ESP32 won't connect to Wi-Fi** | ESP32 only supports **2.4 GHz Wi-Fi** (does not connect to 5 GHz-only bands). If using mobile hotspot, ensure "Maximize Compatibility" / 2.4 GHz is turned ON. |
