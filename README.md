# 🏍️ Bajaj Pulsar NS 200 - DIY Carburetor Air/Fuel Tuning Monitor

ESP32, MQ-2, සහ MQ-135 සෙන්සර් යොදාගෙන සාදන ලද Exhaust Sniffer Gadget එක.

---

## 📌 Features (විශේෂාංග)
1. **Serial Plotter Support:** Laptop එකට USB ගැහුවම Arduino IDE එකේ **Serial Plotter** (`Ctrl + Shift + L`) මඟින් Live Graph එකක් ලෙස බලාගත හැක.
2. **Mobile Web Dashboard (WiFi AP):** Laptop එක බයික් එක ළඟට අරන් යන්න ඕන නැත. ESP32 එකෙන් හැදෙන WiFi එකට ෆෝන් එක connect කර Browser එකෙන් Live Graph සහ Gauges බලාගත හැක (Internet අවශ්‍ය නැත).
3. **Ignition Noise Suppression:** බයික් එකේ Spark plug එකෙන් එන විදුලි ශබ්ද (EMI noise) අවම කිරීමට Digital Averaging Filter එකක් අඩංගුය.

---

## 🔌 Pin Connections (වයර් සම්බන්ධ කරන ආකාරය)

| Sensor Pin | ESP32 Pin | විස්තරය |
| :--- | :--- | :--- |
| **MQ-2 VCC** | **5V (VIN)** | 5V Power (Internal heater එක සඳහා) |
| **MQ-2 GND** | **GND** | Ground |
| **MQ-2 A0** | **GPIO 34** | Analog Out (Unburnt Fuel / Hydrocarbons) |
| **MQ-135 VCC**| **5V (VIN)** | 5V Power |
| **MQ-135 GND**| **GND** | Ground |
| **MQ-135 A0** | **GPIO 35** | Analog Out (CO2 / NOx / Smoke) |

> ⚠️ **සටහන:** සෙන්සර් දෙකේම **D0** පින් එක connect නොකර හිස්ව තබන්න.

---

## 🛠️ How to Upload & Use (භාවිතා කරන ආකාරය)

### 1. Code එක Upload කිරීම:
1. Arduino IDE එකෙන් [CO2_Test.ino](file:///Users/Ashan/my%20project/CO2%20Test/CO2_Test.ino) ගොනුව open කරගන්න.
2. **Tools > Board** වෙතින් `DOIT ESP32 DEVKIT V1` (හෝ ඔබගේ ESP32 board එක) තෝරන්න.
3. **Upload** කරන්න.

### 2. Phone එකෙන් Live Dashboard එක බැලීම (හොඳම ක්‍රමය):
1. ESP32 එක Power On කරන්න (Power Bank එකකින් හෝ 5V Charger එකකින්).
2. ෆෝන් එකේ Wi-Fi On කර **`NS200_Carb_Tuner`** නමැති Wi-Fi එකට connect වන්න:
   * **Password:** `12345678`
3. ෆෝන් එකේ Chrome හෝ Safari browser එක open කර පහත ලිපිනයට යන්න:
   * **`http://192.168.4.1`**
4. දැන් ඔබට ෆෝන් එකෙන් Live Graph එක සහ Lean / Rich තත්ත්වය පෙනෙනු ඇත.

### 3. Laptop එකෙන් Serial Plotter බැලීම:
* Baud Rate එක **`115200`** ලෙස තබා Arduino IDE එකේ **Tools > Serial Plotter** (`Ctrl + Shift + L`) ඔබන්න.

---

## 🎯 Calibrated Tuning Scale (Ashan's NS 200 Setup):

* **Clean Air Baseline:** `~72`
* **Pure Combustible Gas Max:** `4095`

| Reading (MQ-2) | Status | Explanation & Action |
| :--- | :--- | :--- |
| **< 150** | **CLEAN AIR** | Probe is not in exhaust or engine is off |
| **150 – 600** | **LEAN** | Mixture too lean (air excess). Turn Pilot Screw OUT (richer) |
| **600 – 1800** | **★ OPTIMAL TUNE ★** | **Target Zone!** Smooth RPM, complete combustion |
| **1800 – 2800** | **SLIGHTLY RICH** | Slightly rich. Turn Pilot Screw IN slightly for better mileage |
| **> 2800** | **VERY RICH** | Excess unburnt fuel & soot. Turn Pilot Screw IN (leaner) |

