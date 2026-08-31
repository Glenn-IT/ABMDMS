# ABMDMS — Project Notes & System Memory

Arduino Based Motion Detection Monitoring System (ABMDMS).
Primary hardware hub, XAMPP web dashboard, and MySQL (`motion_monitoring`) database backend.

---

## ⚡ SISTER PROJECT INTEGRATION: Android Studio MBPSAAS

This project shares hardware definitions, database, and serial protocols with the Android Studio mobile app project located at:
`C:\Users\GLENN\AndroidStudioProjects\MBPSAASMobileBasedPoultrySecurityandAlertSystem\`

### Critical Shared Specs:
- **Database:** `motion_monitoring` (tables: `motion_logs`, `sms_logs`, `users`, `sensor_zones`).
- **Pins & Zones:**
  - Pin 2: `ROOMC` (Coop Zone C)
  - Pin 3: `ROOMA` (Coop Zone A)
  - Pin 4: `ROOMB` (Coop Zone B)
  - Pin 8: Farm Alarm Buzzer
  - Pins 10/11/12: SIM800L GSM Module
- **When modifying Arduino pins, zones, or serial outputs here:**
  - Remember to keep `arduino/poultry_sensor/poultry_sensor.ino` and Android models updated in the Android Studio project.
