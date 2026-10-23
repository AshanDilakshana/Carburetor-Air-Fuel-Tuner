/*
  ==============================================================
  DIY Carburetor Air/Fuel Tuning Monitor
  ==============================================================
  Hardware:
    - ESP32 Development Board
    - MQ-2 Gas & Smoke Sensor (Unburnt Fuel / Hydrocarbons)
    - MQ-135 Air Quality Sensor (Exhaust Index / CO2 / NOx)

  Pin Connections:
    - MQ-2   A0 -> ESP32 GPIO 34 (Analog In)
    - MQ-135 A0 -> ESP32 GPIO 35 (Analog In)
    - MQ-2   VCC -> 5V (VIN)
    - MQ-135 VCC -> 5V (VIN)
    - GND        -> ESP32 GND
    *(D0 pins are left unconnected)*

  Features:
    1. Dual Serial Output (Serial Monitor & Serial Plotter compatible)
    2. Built-in WiFi Access Point ("NS200_Carb_Tuner")
    3. Mobile Web Dashboard (192.168.4.1) with live Real-time Graphs & Gauges
       - Works 100% offline (no internet needed)
    4. Anti-Noise Filtering (Averaging filter to suppress motorcycle spark EMI)
  ==============================================================
*/

#include <ESPmDNS.h>
#include <WebServer.h>
#include <WiFi.h>

// Pin Configuration
#define MQ2_PIN 34
#define MQ135_PIN 35

// Built-in LED (GPIO 2 on most ESP32 boards)
#define LED_PIN 2

// User Wi-Fi Hotspot Credentials
const char *WIFI_SSID = "Faaa";
const char *WIFI_PASSWORD = "11223344";

WebServer server(80);

// Sensor Variables
int rawMQ2 = 0;
int rawMQ135 = 0;
float voltMQ2 = 0.0;
float voltMQ135 = 0.0;
String tuneStatus = "INITIALIZING";

// Warmup timer
unsigned long startTime = 0;
const unsigned long WARMUP_TIME_MS = 60000; // 1 min initial warmup display

// Filtered ADC reading (reduces bike ignition noise)
int readAveragedADC(int pin, int samples = 20) {
  long sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(pin);
    delayMicroseconds(400);
  }
  return sum / samples;
}

// Dual-Sensor Fusion Algorithm (Combines MQ-2 Unburnt Fuel + MQ-135 Combustion
// Index)
void evaluateTune() {
  unsigned long elapsed = millis() - startTime;
  if (elapsed < WARMUP_TIME_MS) {
    tuneStatus = "WARMING UP";
    return;
  }

  // Probe in clean air or bike off
  if (rawMQ2 < 150 && rawMQ135 < 150) {
    tuneStatus = "CLEAN AIR (Probe not in exhaust)";
    return;
  }

  // Special Diagnostic: High fuel + very low CO2 means fuel is not burning!
  if (rawMQ2 > 2000 && rawMQ135 < 350) {
    tuneStatus = "MISFIRE DETECTED (Fuel unburnt - check spark plug)";
    return;
  }

  // Weighted Composite Tuning Score (70% Fuel, 30% Exhaust Density)
  int tuneScore = (int)(rawMQ2 * 0.70 + rawMQ135 * 0.30);

  if (tuneScore < 600) {
    tuneStatus = "LEAN (Too Much Air / Fuel Starved)";
  } else if (tuneScore <= 1800) {
    tuneStatus = "★ OPTIMAL TUNE ★ (Perfect Stoichiometric)";
  } else if (tuneScore <= 2800) {
    tuneStatus = "SLIGHTLY RICH (Good response, high fuel)";
  } else {
    tuneStatus = "VERY RICH (Excess Fuel & Soot)";
  }
}

// JSON endpoint for mobile web dashboard
void handleData() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  String json = "{";
  json += "\"mq2\":" + String(rawMQ2) + ",";
  json += "\"mq135\":" + String(rawMQ135) + ",";
  json += "\"volt2\":" + String(voltMQ2, 2) + ",";
  json += "\"volt135\":" + String(voltMQ135, 2) + ",";
  json += "\"status\":\"" + tuneStatus + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

// Offline Embedded HTML/CSS/JS Dashboard
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Carburetor Air/Fuel Tuner</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; }
    body { background: #0b0f19; color: #f8fafc; padding: 14px; min-height: 100vh; display: flex; flex-direction: column; align-items: center; }
    .container { max-width: 650px; width: 100%; display: flex; flex-direction: column; gap: 14px; }
    
    header { text-align: center; padding: 6px 0; }
    h1 { font-size: 1.4rem; color: #38bdf8; font-weight: 800; letter-spacing: 0.5px; }
    .sub { font-size: 0.78rem; color: #94a3b8; margin-top: 2px; }

    /* Main Range Meter Card */
    .meter-card {
      background: #161e31; border: 2px solid rgba(56, 189, 248, 0.25); border-radius: 16px; padding: 18px 16px;
      box-shadow: 0 8px 30px rgba(0,0,0,0.5); text-align: center;
    }

    .trend-badge {
      display: inline-flex; align-items: center; gap: 6px; padding: 5px 12px; border-radius: 999px;
      font-size: 0.8rem; font-weight: 700; margin-bottom: 12px; background: rgba(255,255,255,0.06);
    }
    .trend-up { color: #ef4444; background: rgba(239, 68, 68, 0.15); border: 1px solid rgba(239, 68, 68, 0.4); }
    .trend-down { color: #f59e0b; background: rgba(245, 158, 11, 0.15); border: 1px solid rgba(245, 158, 11, 0.4); }
    .trend-stable { color: #10b981; background: rgba(16, 185, 129, 0.15); border: 1px solid rgba(16, 185, 129, 0.4); }

    .status-text { font-size: 1.8rem; font-weight: 900; letter-spacing: 0.5px; margin-bottom: 4px; }
    .status-desc { font-size: 0.85rem; color: #94a3b8; margin-bottom: 24px; min-height: 20px; }

    /* Multi-Zone Range Bar */
    .range-box { position: relative; margin: 30px 4px 16px 4px; }
    .needle {
      position: absolute; top: -24px; left: 0%; transform: translateX(-50%);
      display: flex; flex-direction: column; align-items: center; transition: left 0.25s ease;
    }
    .needle-box { background: #fff; color: #0b0f19; font-weight: 900; font-size: 0.75rem; padding: 2px 7px; border-radius: 5px; }
    .needle-tip { width: 0; height: 0; border-left: 6px solid transparent; border-right: 6px solid transparent; border-top: 7px solid #fff; }

    .track { height: 20px; border-radius: 10px; background: #0f172a; display: flex; overflow: hidden; border: 1px solid rgba(255,255,255,0.15); }
    .zone { height: 100%; display: flex; align-items: center; justify-content: center; font-size: 0.62rem; font-weight: 800; color: #fff; }
    .z-lean { width: 22%; background: linear-gradient(90deg, #d97706, #f59e0b); }
    .z-opt { width: 28%; background: linear-gradient(90deg, #059669, #10b981); box-shadow: 0 0 10px rgba(16,185,129,0.5); }
    .z-srich { width: 22%; background: linear-gradient(90deg, #f87171, #ef4444); }
    .z-rich { width: 28%; background: linear-gradient(90deg, #dc2626, #991b1b); }

    .scale { display: flex; justify-content: space-between; font-size: 0.68rem; color: #94a3b8; font-weight: 600; margin-top: 6px; }

    /* Values Grid */
    .grid { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; }
    .card { background: #161e31; border: 1px solid rgba(56, 189, 248, 0.15); border-radius: 14px; padding: 14px; text-align: center; }
    .card-title { font-size: 0.75rem; color: #94a3b8; margin-bottom: 2px; }
    .card-val { font-size: 2rem; font-weight: 800; line-height: 1; }
    .card-volt { font-size: 0.75rem; color: #64748b; margin-top: 4px; }

    /* Chart */
    .chart-box { background: #161e31; border: 1px solid rgba(56, 189, 248, 0.15); border-radius: 14px; padding: 12px; }
    .chart-head { font-size: 0.75rem; color: #94a3b8; margin-bottom: 6px; display: flex; justify-content: space-between; }
    canvas { width: 100%; height: 140px; background: #060a12; border-radius: 8px; display: block; }
  </style>
</head>
<body>
  <div class="container">
    <header>
      <h1>Carburetor Live Monitor</h1>
      <div class="sub">Direct Wi-Fi Tuning Dashboard</div>
    </header>

    <!-- Range & Tuning Level Card -->
    <div class="meter-card">
      <div id="trendBadge" class="trend-badge trend-stable">
        <span id="trendIcon">▬</span>
        <span id="trendText">Stable</span>
      </div>

      <div id="statusTitle" class="status-text" style="color: #10b981;">INITIALIZING...</div>
      <div id="statusDesc" class="status-desc">Waiting for sensor data...</div>

      <div class="range-box">
        <div class="needle" id="needle">
          <div class="needle-box" id="needleVal">0</div>
          <div class="needle-tip"></div>
        </div>
        <div class="track">
          <div class="zone z-lean" style="width: 15%;">LEAN</div>
          <div class="zone z-opt" style="width: 30%;">★ OPTIMAL ★</div>
          <div class="zone z-srich" style="width: 25%;">SLIGHT RICH</div>
          <div class="zone z-rich" style="width: 30%;">VERY RICH</div>
        </div>
        <div class="scale">
          <span>72 (Clean)</span>
          <span>600 (Lean Limit)</span>
          <span>1800 (Opt Limit)</span>
          <span>2800 (Rich Limit)</span>
          <span>4095 (Max)</span>
        </div>
      </div>
    </div>

    <!-- Values Grid -->
    <div class="grid">
      <div class="card">
        <div class="card-title">MQ-2 (UNBURNT FUEL)</div>
        <div class="card-val" id="mq2Val" style="color: #f97316;">0</div>
        <div class="card-volt" id="mq2Volt">0.00 V</div>
      </div>
      <div class="card">
        <div class="card-title">MQ-135 (EXHAUST)</div>
        <div class="card-val" id="mq135Val" style="color: #a855f7;">0</div>
        <div class="card-volt" id="mq135Volt">0.00 V</div>
      </div>
    </div>

    <!-- Live Waveform -->
    <div class="chart-box">
      <div class="chart-head">
        <span>Live Waveform</span>
        <span><span style="color: #f97316;">■ MQ-2</span> &nbsp; <span style="color: #a855f7;">■ MQ-135</span></span>
      </div>
      <canvas id="chart"></canvas>
    </div>
  </div>

  <script>
    const canvas = document.getElementById('chart');
    const ctx = canvas.getContext('2d');
    canvas.width = canvas.offsetWidth;
    canvas.height = canvas.offsetHeight;

    const maxPoints = 50;
    const mq2History = new Array(maxPoints).fill(0);
    const mq135History = new Array(maxPoints).fill(0);

    let prevMQ2 = 0;
    let lastTrendCheck = Date.now();

    function drawChart() {
      ctx.clearRect(0, 0, canvas.width, canvas.height);
      ctx.strokeStyle = 'rgba(255,255,255,0.06)';
      ctx.beginPath();
      ctx.moveTo(0, canvas.height/2);
      ctx.lineTo(canvas.width, canvas.height/2);
      ctx.stroke();

      function plot(data, color) {
        ctx.strokeStyle = color;
        ctx.lineWidth = 2;
        ctx.beginPath();
        for (let i = 0; i < data.length; i++) {
          const x = (i / (maxPoints - 1)) * canvas.width;
          const y = canvas.height - ((data[i] / 4095) * canvas.height);
          if (i === 0) ctx.moveTo(x, y);
          else ctx.lineTo(x, y);
        }
        ctx.stroke();
      }
      plot(mq135History, '#a855f7');
      plot(mq2History, '#f97316');
    }

    async function fetchData() {
      try {
        const res = await fetch('/data');
        const d = await res.json();

        // 1. Raw & Voltages
        document.getElementById('mq2Val').innerText = d.mq2;
        document.getElementById('mq2Volt').innerText = d.volt2 + ' V';
        document.getElementById('mq135Val').innerText = d.mq135;
        document.getElementById('mq135Volt').innerText = d.volt135 + ' V';

        // 2. Needle Movement on the Range Bar (Dual-Sensor Fusion: 70% Fuel + 30% Exhaust Density)
        const compositeScore = Math.round((d.mq2 * 0.70) + (d.mq135 * 0.30));
        const pct = Math.min(100, Math.max(0, (compositeScore / 4095) * 100));
        document.getElementById('needle').style.left = pct + '%';
        document.getElementById('needleVal').innerText = compositeScore;

        // 3. Trend: Increase / Decrease
        const now = Date.now();
        if (now - lastTrendCheck > 400) {
          const delta = d.mq2 - prevMQ2;
          const badge = document.getElementById('trendBadge');
          const icon = document.getElementById('trendIcon');
          const text = document.getElementById('trendText');

          if (delta > 30) {
            badge.className = 'trend-badge trend-up';
            icon.innerText = '▲';
            text.innerText = `Fuel Increasing (+${delta}/s)`;
          } else if (delta < -30) {
            badge.className = 'trend-badge trend-down';
            icon.innerText = '▼';
            text.innerText = `Fuel Decreasing (${delta}/s)`;
          } else {
            badge.className = 'trend-badge trend-stable';
            icon.innerText = '▬';
            text.innerText = 'Stable Idle';
          }
          prevMQ2 = d.mq2;
          lastTrendCheck = now;
        }

        // 4. Dual-Sensor Status Title & Smart Guidance
        const titleEl = document.getElementById('statusTitle');
        const descEl = document.getElementById('statusDesc');

        if (d.mq2 < 150 && d.mq135 < 150) {
          titleEl.innerText = "CLEAN AIR / READY";
          titleEl.style.color = "#38bdf8";
          descEl.innerText = "Probe in clean air. Insert into exhaust and start engine.";
        } else if (d.mq2 > 2000 && d.mq135 < 350) {
          titleEl.innerText = "⚠️ MISFIRE DETECTED";
          titleEl.style.color = "#ec4899";
          descEl.innerText = "High unburnt fuel but low CO2! Fuel not burning. Check spark plug.";
        } else if (compositeScore < 600) {
          titleEl.innerText = "LEAN (TOO MUCH AIR)";
          titleEl.style.color = "#f59e0b";
          descEl.innerText = "Mixture too lean / engine running hot. Turn Pilot Screw OUT (Outward).";
        } else if (compositeScore <= 1800) {
          titleEl.innerText = "★ OPTIMAL TUNE ★";
          titleEl.style.color = "#10b981";
          descEl.innerText = "Optimal stoichiometric mixture achieved! Clean burn with healthy CO2.";
        } else if (compositeScore <= 2800) {
          titleEl.innerText = "SLIGHTLY RICH";
          titleEl.style.color = "#f87171";
          descEl.innerText = "Slightly rich. Turn Pilot Screw IN slightly for best mileage.";
        } else {
          titleEl.innerText = "VERY RICH (EXCESS FUEL)";
          titleEl.style.color = "#ef4444";
          descEl.innerText = "Excess unburnt fuel & soot. Turn Pilot Screw IN (Inward) to lean.";
        }

        mq2History.shift();
        mq2History.push(d.mq2);
        mq135History.shift();
        mq135History.push(d.mq135);

        drawChart();
      } catch (e) {
        console.error(e);
      }
    }

    setInterval(fetchData, 250);
    window.addEventListener('resize', () => {
      canvas.width = canvas.offsetWidth;
      canvas.height = canvas.offsetHeight;
    });
  </script>
</body>
</html>
)rawliteral";

void handleRoot() { server.send(200, "text/html", INDEX_HTML); }

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // ESP32 12-bit ADC (0 - 4095)
  analogReadResolution(12);

  startTime = millis();

  // Dual-Mode: Connect to "Faaa" AND broadcast "NS200_Carb_Tuner" simultaneously!
  Serial.println("\n==============================================");
  Serial.println("[+] Starting Dual Wi-Fi (Hotspot + Client)...");
  Serial.println("==============================================");

  WiFi.mode(WIFI_AP_STA);

  // 1. Guaranteed Fallback Hotspot (Always Available!)
  WiFi.softAP("NS200_Carb_Tuner", "12345678");
  Serial.print("[+] Fallback AP Active! SSID: NS200_Carb_Tuner (Password: 12345678)");
  Serial.print("\n[+] Direct IP: http://");
  Serial.println(WiFi.softAPIP());

  // 2. Attempt Connection to User's Hotspot ("Faaa")
  Serial.print("\n[+] Attempting connection to 'Faaa'...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 25) {
    delay(400);
    digitalWrite(LED_PIN, !digitalRead(LED_PIN)); // Blink
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(LED_PIN, HIGH);
    Serial.println("\n\n>>> [SUCCESS] CONNECTED TO 'Faaa'! <<<");
    Serial.print(">>> ACCESS VIA 'Faaa' IP: http://");
    Serial.println(WiFi.localIP());
    Serial.println(">>> OR VIA MDNS:         http://ns200.local");
  } else {
    digitalWrite(LED_PIN, LOW);
    Serial.println("\n\n[!] Could not connect to 'Faaa' (Likely 5GHz band or hotspot off).");
    Serial.println("[!] NO PROBLEM! You can connect to ESP32 Wi-Fi directly:");
    Serial.println("    SSID:     NS200_Carb_Tuner");
    Serial.println("    Password: 12345678");
    Serial.println("    IP:       http://192.168.4.1");
  }
  Serial.println("==============================================\n");

  if (MDNS.begin("ns200")) {
    MDNS.addService("http", "tcp", 80);
  }

  // Setup Web Server Routes
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.enableCORS();
  server.begin();
  Serial.println("[+] HTTP Web Server running!\n");
}

void loop() {
  server.handleClient();

  // Auto-reconnect if Wi-Fi connection drops
  static unsigned long lastWifiCheck = 0;
  if (millis() - lastWifiCheck > 10000) {
    lastWifiCheck = millis();
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("[!] Wi-Fi lost. Reconnecting to " + String(WIFI_SSID) +
                     "...");
      WiFi.reconnect();
    }
  }

  // Read filtered sensor values
  rawMQ2 = readAveragedADC(MQ2_PIN);
  rawMQ135 = readAveragedADC(MQ135_PIN);

  // Calculate voltage (ESP32 ADC reference ~3.3V)
  voltMQ2 = (rawMQ2 / 4095.0) * 3.3;
  voltMQ135 = (rawMQ135 / 4095.0) * 3.3;

  evaluateTune();

  // Serial Output for Arduino Serial Plotter
  // Open Serial Plotter (Ctrl + Shift + L) at 115200 baud
  static unsigned long lastSerialPrint = 0;
  if (millis() - lastSerialPrint >= 150) {
    lastSerialPrint = millis();
    Serial.print("MQ2_Unburnt_Fuel:");
    Serial.print(rawMQ2);
    Serial.print(",");
    Serial.print("MQ135_Exhaust_Index:");
    Serial.print(rawMQ135);
    Serial.print(",");
    Serial.print("Tune_State:");
    Serial.println(rawMQ2); // Numerical anchor for plotter
  }
}
