# 🏍️ DIY Carburetor Air/Fuel Tuning Monitor

[![ESP32](https://img.shields.io/badge/Hardware-ESP32-blue.svg)](https://espressif.com/)
[![Arduino](https://img.shields.io/badge/IDE-Arduino-00979D.svg)](https://www.arduino.cc/)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![GitHub](https://img.shields.io/badge/GitHub-AshanDilakshana-181717.svg)](https://github.com/AshanDilakshana/Carburetor-Air-Fuel-Tuner)

An intelligent DIY motorcycle and small engine exhaust gas sniffer and carburetor tuning monitor built using **ESP32**, **MQ-2** (Unburnt Fuel / Hydrocarbons), and **MQ-135** (Exhaust Gas Density / CO2 / NOx). Features real-time **Dual-Sensor Fusion**, a standalone **Web Serial USB Dashboard**, and built-in **Wi-Fi Web Server**.

---

## 📌 Key Features

* **Dual-Sensor Fusion Algorithm:** Combines raw readings from both MQ-2 (70% weight for fuel richness) and MQ-135 (30% weight for combustion efficiency) to accurately assess the engine's stoichiometric balance.
* **Misfire & Spark Fault Detection:** Automatically flags unburnt fuel spikes accompanied by low CO2 output as an ignition misfire.
* **Dual-Mode Wi-Fi Architecture:** Connects to your local Wi-Fi / smartphone hotspot (`Faaa`) while simultaneously maintaining a standalone fallback access point (`Carb_Tuner_AP`) at `192.168.4.1`.
* **Zero-Install Web Serial Dashboard (`index.html`):** Connects directly to the ESP32 via Google Chrome or Microsoft Edge USB Serial API (no Arduino IDE or software installation required on the client machine).
* **Ignition Noise Suppression:** Embedded moving-average digital filter dampens spark plug electromagnetic interference (EMI).
* **Arduino Serial Plotter Compatibility:** Streams real-time formatted data over 115200 baud for immediate graph visualization in Arduino IDE (`Ctrl + Shift + L`).

---

## 🔌 Hardware Wiring & Pin Connections

| Sensor Pin | ESP32 Pin | Function | Notes |
| :--- | :--- | :--- | :--- |
| **MQ-2 VCC** | **VIN (5V)** | Power Supply | 5V required for internal heater coil |
| **MQ-2 GND** | **GND** | Ground | Common system ground |
| **MQ-2 A0** | **GPIO 34** | Analog Input (ADC1) | Measures unburnt hydrocarbons & fuel vapor |
| **MQ-135 VCC**| **VIN (5V)** | Power Supply | 5V required for internal heater coil |
| **MQ-135 GND**| **GND** | Ground | Common system ground |
| **MQ-135 A0** | **GPIO 35** | Analog Input (ADC1) | Measures CO2, smoke, and exhaust density |

> ⚠️ **Note:** Leave the **D0** (Digital Output) pins on both MQ sensor breakout boards unconnected. Only the **A0** (Analog Output) pins are used.

---

## 🧪 Mechanical Exhaust Sniffer Setup

Do not place MQ sensors directly inside the hot exhaust pipe. Follow this simple cooling probe layout:

```text
[Silencer Tip]
      │
      ▼
[Copper / Metal Tube: ~15 cm] (Withstands exhaust heat)
      │
      ▼
[Heat-resistant Silicone Hose: 1 - 1.5 m] (Cools exhaust gas to ambient temp)
      │
      ▼
[Inline Fuel Filter / Cotton Trap] (Stops soot and condensation droplets)
      │
      ▼
[Sensor Chamber with MQ-2 & MQ-135]
      │
      ▼
[Exhaust Vent Hole] (Ensures smooth gas flow-through)
```

---

## 🎯 Calibrated Tuning Scale

* **Clean Air Baseline (Stabilized):** `~50 - 75`
* **Pure Fuel Vapor Saturation (Max):** `4095`

$$\text{Composite Score} = (\text{MQ-2} \times 0.70) + (\text{MQ-135} \times 0.30)$$

| Composite Score | Status | Diagnosis & Tuning Action |
| :---: | :---: | :--- |
| **< 150** | 🔵 **CLEAN AIR / READY** | Probe is not in exhaust or engine is off. |
| **150 – 600** | 🟡 **LEAN (TOO MUCH AIR)** | Mixture too lean. Engine runs hot. Turn Pilot Screw **OUT** (Richer). |
| **600 – 1800** | 🟢 **★ OPTIMAL TUNE ★** | **Target Zone!** Complete combustion, highest smooth idle RPM. |
| **1800 – 2800** | 🔴 **SLIGHTLY RICH** | Good throttle response, slightly high fuel consumption. Turn Pilot Screw **IN** slightly. |
| **> 2800** | 🛑 **VERY RICH** | Heavy unburnt fuel and soot. Turn Pilot Screw **IN** (Leaner). |
| *Special Condition* | 🟣 **MISFIRE DETECTED** | High MQ-2 (> 2000) with low MQ-135 (< 350). Check spark plug and ignition coil. |

---

## 🛠️ How to Upload & Use

### 1. Flash the ESP32 Firmware
1. Open [`CO2_Test/CO2_Test.ino`](CO2_Test/CO2_Test.ino) in Arduino IDE.
2. Select your board from **Tools > Board > ESP32 Dev Module**.
3. Select your serial port under **Tools > Port**.
4. Click **Upload (➔)**.

### 2. View the Live Web Dashboard
* **Option A: USB Serial Mode (Easiest on Laptop):**
  1. Close the Arduino Serial Monitor window.
  2. Open [`index.html`](index.html) in Google Chrome or Microsoft Edge.
  3. Click **🔌 USB Serial** and select your ESP32 serial port.
* **Option B: Wi-Fi Mode (Phone / Wireless):**
  1. Connect your phone or laptop to the Wi-Fi network `Faaa` or directly to `Carb_Tuner_AP` (Password: `12345678`).
  2. Open your web browser and navigate to `http://192.168.4.1` or `http://ns200.local`.

---

## 👤 Author
Developed and maintained by **[Ashan Dilakshana](https://github.com/AshanDilakshana)**.
