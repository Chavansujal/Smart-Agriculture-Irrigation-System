#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// ---------- WiFi Credentials ----------
const char* ssid = "vivo Y33T";
const char* password = "13271329";

// ---------- AI Model Server Configuration ----------
char aiServerIP[64] = "10.185.32.62";  // Change to your PC's IP address or update via Web UI
int aiServerPort = 8000;
bool aiServerConnected = false;
unsigned long lastAIServerSync = 0;
const unsigned long aiSyncInterval = 2000; // Sync with AI Server every 2s

// AI Model Response State
float aiPredictedWater = 0.0;
String aiAction = "PUMP_OFF";
String aiReason = "Connecting to AI Model...";
float aiRuntimeSec = 0.0;

// ---------- Operational Mode ----------
// Modes: "AI_AUTO", "THRESHOLD_AUTO", "MANUAL"
String currentMode = "AI_AUTO";
String selectedCrop = "Wheat";
String selectedSeason = "Winter";

// ---------- Google Apps Script Web App URL ----------
const char* scriptURL = "https://script.google.com/macros/s/AKfycbzRao3vob-56062OIwk0oN_Gqa73Pd0_Kowwv0Pc_FW5zLNvgNadjmDHvF0nJhuDfo6AA/exec";

// ---------- Pin Definitions ----------
#define DHTPIN 4
#define DHTTYPE DHT11
#define LDR_PIN 5
#define RAIN_PIN 6
#define SOIL_PIN 7
#define WATERLEVEL_PIN 2
#define UNO_SIGNAL_PIN 11
#define SDA_PIN 8
#define SCL_PIN 9

LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHTPIN, DHTTYPE);
WebServer server(80);

float temperature = 0;
float humidity = 0;
int lightIntensity = 0;
int rainStatus = HIGH;
int soilValue = 0;
int waterLevel = 0;
bool relayOn = false;

// Auto irrigation threshold
int soilThreshold = 2500; // above this = dry

unsigned long lastRead = 0;
const unsigned long readInterval = 2000;
unsigned long lastLcdSwitch = 0;
int lcdScreen = 0;

unsigned long lastSheetSend = 0;
const unsigned long sheetSendInterval = 15000;

// ---------- Web Dashboard HTML Page ----------
const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ESP32 Smart Agriculture AI Dashboard</title>
<style>
  * { box-sizing: border-box; }
  body { font-family: 'Segoe UI', Arial, sans-serif; background:#0b1329; color:#e2e8f0; margin:0; padding:16px; }
  h1 { text-align:center; color:#10b981; font-size:1.4rem; margin-bottom:4px; font-weight:700; }
  .status { text-align:center; font-size:0.8rem; margin-bottom:16px; display:flex; justify-content:center; align-items:center; gap:8px; }
  .dot { display:inline-block; width:10px; height:10px; border-radius:50%; background:#10b981; animation:pulse 1.5s infinite; }
  .dot.red { background:#f43f5e !important; }
  @keyframes pulse { 0%{opacity:1;} 50%{opacity:0.3;} 100%{opacity:1;} }
  
  .config-bar { max-width:680px; margin:0 auto 16px; background:#1e293b; border-radius:12px; padding:12px 16px; display:flex; gap:10px; align-items:center; border:1px solid #334155; }
  .config-bar input { flex:1; background:#0f172a; border:1px solid #334155; border-radius:8px; color:#fff; padding:8px 12px; font-size:0.85rem; }
  .config-bar button { background:#10b981; color:#000; border:none; border-radius:8px; padding:8px 14px; font-weight:bold; cursor:pointer; }
  
  .grid { display:grid; grid-template-columns: repeat(auto-fit, minmax(130px,1fr)); gap:12px; max-width:680px; margin:0 auto 16px; }
  .card { background:#1e293b; border-radius:12px; padding:14px; text-align:center; border:1px solid #334155; transition: transform 0.15s; }
  .card:hover { transform: translateY(-2px); }
  .card .label { font-size:0.75rem; color:#94a3b8; margin-bottom:6px; }
  .card .value { font-size:1.4rem; font-weight:bold; }
  .warn { background:#7f1d1d !important; border-color:#ef4444 !important; }
  .rain-yes { color:#f87171; }
  .rain-no { color:#4ade80; }
  
  .ai-panel { max-width:680px; margin:0 auto 16px; background:linear-gradient(135deg, rgba(16,185,129,0.15), rgba(6,182,212,0.15)); border:1px solid #10b981; border-radius:14px; padding:18px; text-align:center; }
  .ai-badge { display:inline-block; padding:6px 16px; border-radius:20px; font-weight:bold; font-size:1.1rem; margin-bottom:8px; }
  .ai-on { background:#10b981; color:#000; box-shadow:0 0 12px rgba(16,185,129,0.4); }
  .ai-off { background:#64748b; color:#fff; }
  .ai-stats { display:grid; grid-template-columns:1fr 1fr 1fr; gap:10px; margin-top:12px; }
  .ai-stat-box { background:rgba(15,23,42,0.6); padding:10px; border-radius:8px; border:1px solid #334155; }
  .ai-stat-box .lbl { font-size:0.7rem; color:#94a3b8; }
  .ai-stat-box .val { font-size:1.1rem; font-weight:bold; color:#38bdf8; margin-top:2px; }
  
  .panel { max-width:680px; margin:0 auto 16px; background:#1e293b; border-radius:12px; padding:16px; border:1px solid #334155; }
  .mode-selector { display:flex; gap:8px; margin-bottom:14px; }
  .mode-btn { flex:1; padding:10px; background:#0f172a; border:1px solid #334155; border-radius:8px; color:#94a3b8; font-size:0.8rem; font-weight:bold; cursor:pointer; }
  .mode-btn.active { background:#10b981; color:#000; border-color:#10b981; }
  
  canvas { width:100%; height:130px; }
  .relay-row { display:flex; justify-content:space-between; align-items:center; max-width:680px; margin:0 auto 16px; background:#1e293b; border-radius:12px; padding:16px; border:1px solid #334155; }
  .switch { position:relative; width:52px; height:28px; flex-shrink:0; }
  .switch input { opacity:0; width:0; height:0; }
  .slider { position:absolute; cursor:pointer; inset:0; background:#334155; border-radius:28px; transition:0.3s; }
  .slider:before { content:""; position:absolute; height:20px; width:20px; left:4px; bottom:4px; background:white; border-radius:50%; transition:0.3s; }
  input:checked + .slider { background:#22c55e; }
  input:checked + .slider:before { transform: translateX(24px); }
  .switch.disabled { opacity:0.4; pointer-events:none; }
  
  .footer { text-align:center; margin-top:10px; color:#64748b; font-size:0.75rem; }
  .sheet-note { text-align:center; font-size:0.72rem; color:#4ade80; margin-top:4px; }
  .alert-btn { display:block; width:100%; max-width:680px; margin:0 auto 16px; background:#334155; color:#e2e8f0; border:none; padding:10px 16px; border-radius:8px; font-size:0.8rem; cursor:pointer; }
  .alert-btn.enabled { background:#166534; }
  .stop-btn { display:none; width:100%; max-width:680px; margin:0 auto 16px; background:#dc2626; color:white; border:none; padding:14px 16px; border-radius:8px; font-size:0.95rem; font-weight:bold; cursor:pointer; animation:pulse 1s infinite; }
  .stop-btn.active { display:block; }
  table { width:100%; border-collapse:collapse; font-size:0.72rem; }
  th, td { padding:6px 4px; text-align:center; border-bottom:1px solid #334155; }
  th { color:#94a3b8; font-weight:normal; }
  .table-wrap { max-height:200px; overflow-y:auto; }
  select, input[type=number] { background:#0f172a; color:#e2e8f0; border:1px solid #334155; border-radius:6px; padding:8px; width:100%; margin-top:4px; }
  label.field { display:block; font-size:0.75rem; color:#94a3b8; margin-bottom:12px; }
  .btn { background:#22c55e; color:#052e16; border:none; padding:10px 16px; border-radius:8px; font-weight:bold; cursor:pointer; width:100%; margin-top:8px; }
  
  .modal-overlay { position:fixed; inset:0; background:rgba(0,0,0,0.7); display:flex; align-items:center; justify-content:center; z-index:100; padding:16px; }
  .modal-box { background:#1e293b; border-radius:14px; padding:24px; max-width:360px; width:100%; border:1px solid #334155; }
  .modal-box h2 { margin-top:0; font-size:1.1rem; color:#4ade80; }
  .change-btn { background:none; border:1px solid #334155; color:#94a3b8; padding:4px 10px; border-radius:6px; font-size:0.7rem; cursor:pointer; }
  .crop-header { display:flex; justify-content:space-between; align-items:center; max-width:680px; margin:0 auto 12px; font-size:0.8rem; color:#94a3b8; }
</style>
</head>
<body>

  <div class="modal-overlay" id="setupModal">
    <div class="modal-box">
      <h2>🌾 Farm Setup</h2>
      <label class="field">Crop type
        <select id="cropSelect">
          <option value="Wheat">Wheat</option>
          <option value="Rice">Rice</option>
          <option value="Maize">Maize</option>
          <option value="Cotton">Cotton</option>
          <option value="Sugarcane">Sugarcane</option>
          <option value="Vegetables">Vegetables</option>
        </select>
      </label>
      <label class="field">Current season
        <select id="seasonSelect">
          <option value="Winter">Winter</option>
          <option value="Summer">Summer</option>
          <option value="Rainy">Rainy</option>
        </select>
      </label>
      <button class="btn" onclick="saveSetup()">Start Monitoring</button>
    </div>
  </div>

  <h1>Smart Agriculture Dashboard</h1>
  <div class="status">
    <span class="dot" id="dot"></span>
    <span id="statusText">ESP32 Hardware Connected</span> | 
    <span style="color:#38bdf8" id="aiStatus">AI Server: Checking...</span>
  </div>

  <div class="config-bar">
    <span style="font-size:0.8rem; color:#94a3b8; white-space:nowrap;">AI Server IP:</span>
    <input type="text" id="aiIpInput" value="192.168.1.100" placeholder="e.g. 192.168.1.50">
    <button onclick="saveAiServerIp()">Save IP</button>
  </div>

  <div class="crop-header">
    <span id="cropSummary">Crop: Wheat | Season: Winter</span>
    <button class="change-btn" onclick="openSetup()">Change</button>
  </div>

  <button class="alert-btn" id="alertBtn" onclick="enableAlerts()">🔔 Tap to enable rain alert sound</button>
  <button class="stop-btn" id="stopBtn" onclick="stopAlarm()">🔇 STOP ALARM</button>

  <!-- Live AI Prediction Display -->
  <div class="ai-panel">
    <div style="font-size:0.8rem; color:#94a3b8; margin-bottom:6px;">🧠 AI Model Live Inference Output</div>
    <div class="ai-badge ai-off" id="aiActionBadge">PUMP_OFF</div>
    <div style="font-size:0.85rem; color:#e2e8f0; margin-bottom:8px;" id="aiReasonText">Reason: Waiting for AI model response...</div>
    <div class="ai-stats">
      <div class="ai-stat-box"><div class="lbl">Water Needed</div><div class="val" id="aiWaterVal">0.0 L</div></div>
      <div class="ai-stat-box"><div class="lbl">Pump Runtime</div><div class="val" id="aiRuntimeVal">0 s</div></div>
      <div class="ai-stat-box"><div class="lbl">Active Mode</div><div class="val" id="aiModeVal" style="color:#10b981;">AI_AUTO</div></div>
    </div>
  </div>

  <!-- Operational Mode Selection -->
  <div class="panel">
    <div class="label" style="color:#94a3b8;font-size:0.8rem;margin-bottom:8px;">⚙️ Operating Mode</div>
    <div class="mode-selector">
      <button class="mode-btn active" id="modeAiBtn" onclick="setMode('AI_AUTO')">🤖 AI Model Auto</button>
      <button class="mode-btn" id="modeThreshBtn" onclick="setMode('THRESHOLD_AUTO')">📏 Threshold Auto</button>
      <button class="mode-btn" id="modeManualBtn" onclick="setMode('MANUAL')">🕹️ Manual</button>
    </div>

    <div id="thresholdSec" style="display:none; margin-top:10px;">
      <label class="field">Soil dryness threshold (higher = drier)
        <input type="number" id="thresholdInput" value="2500">
      </label>
      <button class="btn" onclick="saveThreshold()">Save Threshold</button>
    </div>
  </div>

  <div class="grid">
    <div class="card" id="tempCard"><div class="label">Temperature</div><div class="value" id="temp">--</div></div>
    <div class="card" id="humCard"><div class="label">Humidity</div><div class="value" id="hum">--</div></div>
    <div class="card" id="lightCard"><div class="label">Light</div><div class="value" id="light">--</div></div>
    <div class="card" id="rainCard"><div class="label">Rain</div><div class="value" id="rain">--</div></div>
    <div class="card" id="soilCard"><div class="label">Soil Moisture</div><div class="value" id="soil">--</div></div>
    <div class="card" id="waterCard"><div class="label">Water Level</div><div class="value" id="water">--</div></div>
  </div>

  <div class="panel">
    <div class="label" style="margin-bottom:8px;color:#94a3b8;font-size:0.75rem;">Temperature trend (last readings)</div>
    <canvas id="chart"></canvas>
  </div>

  <div class="panel">
    <div class="label" style="margin-bottom:8px;color:#94a3b8;font-size:0.75rem;">Reading History (last 15)</div>
    <div class="table-wrap">
      <table>
        <thead><tr><th>Time</th><th>Temp</th><th>Hum</th><th>Light</th><th>Rain</th><th>Soil</th><th>Water</th></tr></thead>
        <tbody id="historyBody"></tbody>
      </table>
    </div>
  </div>

  <div class="relay-row">
    <div>
      <div style="font-weight:bold;">Relay / Pump Output</div>
      <div style="font-size:0.75rem;color:#94a3b8;" id="relayLabel">OFF</div>
    </div>
    <label class="switch" id="relaySwitchWrap">
      <input type="checkbox" id="relayToggle" onchange="toggleRelay()">
      <span class="slider"></span>
    </label>
  </div>

  <div class="footer">Last updated: <span id="lastUpdate">--</span></div>
  <div class="sheet-note">Logging to Google Sheets & AI API every 2s</div>

<script>
let history = [];
let logRows = [];
const maxPoints = 20;
const maxLogRows = 15;
let alertsEnabled = false;
let audioCtx = null;
let alarmInterval = null;
let alarmSilenced = false;

function openSetup() {
  document.getElementById('setupModal').style.display = 'flex';
  const saved = JSON.parse(localStorage.getItem('farmSetup') || '{}');
  if (saved.crop) document.getElementById('cropSelect').value = saved.crop;
  if (saved.season) document.getElementById('seasonSelect').value = saved.season;
}

function saveSetup() {
  const crop = document.getElementById('cropSelect').value;
  const season = document.getElementById('seasonSelect').value;
  localStorage.setItem('farmSetup', JSON.stringify({ crop, season }));
  document.getElementById('setupModal').style.display = 'none';
  updateCropSummary();
  fetch('/setcrop?crop=' + crop + '&season=' + season);
}

function updateCropSummary() {
  const saved = JSON.parse(localStorage.getItem('farmSetup') || '{}');
  if (saved.crop && saved.season) {
    document.getElementById('cropSummary').innerText =
      'Crop: ' + saved.crop + ' | Season: ' + saved.season;
  }
}

function enableAlerts() {
  audioCtx = new (window.AudioContext || window.webkitAudioContext)();
  alertsEnabled = true;
  document.getElementById('alertBtn').innerText = '🔔 Rain alerts enabled';
  document.getElementById('alertBtn').className = 'alert-btn enabled';
  playSingleBeep();
}

function playSingleBeep() {
  if (!audioCtx) return;
  const osc = audioCtx.createOscillator();
  const gain = audioCtx.createGain();
  osc.type = 'sine';
  osc.frequency.value = 880;
  gain.gain.value = 0.25;
  osc.connect(gain);
  gain.connect(audioCtx.destination);
  osc.start();
  setTimeout(() => osc.stop(), 250);
}

function startAlarm() {
  if (alarmInterval) return;
  document.getElementById('stopBtn').className = 'stop-btn active';
  playSingleBeep();
  alarmInterval = setInterval(playSingleBeep, 800);
}

function stopAlarmInternal() {
  if (alarmInterval) {
    clearInterval(alarmInterval);
    alarmInterval = null;
  }
  document.getElementById('stopBtn').className = 'stop-btn';
}

function stopAlarm() {
  alarmSilenced = true;
  stopAlarmInternal();
}

function addHistoryRow(d) {
  const time = new Date().toLocaleTimeString();
  logRows.unshift({ time, t: d.temperature.toFixed(1), h: d.humidity.toFixed(0), l: d.light, r: d.rain ? 'Yes' : 'No', s: d.soil, w: d.water });
  if (logRows.length > maxLogRows) logRows.pop();

  const body = document.getElementById('historyBody');
  body.innerHTML = logRows.map(row =>
    `<tr><td>${row.time}</td><td>${row.t}</td><td>${row.h}</td><td>${row.l}</td><td>${row.r}</td><td>${row.s}</td><td>${row.w}</td></tr>`
  ).join('');
}

async function fetchData() {
  try {
    const res = await fetch('/data');
    const d = await res.json();

    document.getElementById('dot').className = 'dot';
    document.getElementById('temp').innerText = d.temperature.toFixed(1) + ' °C';
    document.getElementById('hum').innerText = d.humidity.toFixed(0) + ' %';
    document.getElementById('light').innerText = d.light;
    document.getElementById('soil').innerText = d.soil;
    document.getElementById('water').innerText = d.water;

    document.getElementById('aiIpInput').value = d.ai_ip || '';
    
    // AI Status Indicator
    const aiStatEl = document.getElementById('aiStatus');
    if (d.ai_connected) {
      aiStatEl.innerText = 'AI Server: Online (' + d.ai_ip + ')';
      aiStatEl.style.color = '#10b981';
    } else {
      aiStatEl.innerText = 'AI Server: Disconnected (' + d.ai_ip + ')';
      aiStatEl.style.color = '#f43f5e';
    }

    // AI Prediction Panel Update
    const badge = document.getElementById('aiActionBadge');
    badge.innerText = d.ai_action;
    badge.className = 'ai-badge ' + (d.ai_action === 'PUMP_ON' ? 'ai-on' : 'ai-off');
    document.getElementById('aiReasonText').innerText = 'Reason: ' + d.ai_reason;
    document.getElementById('aiWaterVal').innerText = d.ai_water.toFixed(1) + ' L';
    document.getElementById('aiRuntimeVal').innerText = d.ai_runtime.toFixed(1) + ' s';
    document.getElementById('aiModeVal').innerText = d.mode;

    // Mode Selector styling
    document.getElementById('modeAiBtn').className = 'mode-btn ' + (d.mode === 'AI_AUTO' ? 'active' : '');
    document.getElementById('modeThreshBtn').className = 'mode-btn ' + (d.mode === 'THRESHOLD_AUTO' ? 'active' : '');
    document.getElementById('modeManualBtn').className = 'mode-btn ' + (d.mode === 'MANUAL' ? 'active' : '');
    document.getElementById('thresholdSec').style.display = (d.mode === 'THRESHOLD_AUTO') ? 'block' : 'none';

    document.getElementById('tempCard').className = d.temperature > 35 ? 'card warn' : 'card';
    document.getElementById('lightCard').className = d.light < 300 ? 'card warn' : 'card';
    document.getElementById('soilCard').className = d.soil < d.threshold ? 'card warn' : 'card';
    document.getElementById('waterCard').className = d.water < 500 ? 'card warn' : 'card';

    const rainEl = document.getElementById('rain');
    const rainCard = document.getElementById('rainCard');
    if (d.rain) {
      rainEl.innerText = 'DETECTED'; rainEl.className = 'value rain-yes'; rainCard.className = 'card warn';
      if (alertsEnabled && !alarmSilenced) startAlarm();
    } else {
      rainEl.innerText = 'NONE'; rainEl.className = 'value rain-no'; rainCard.className = 'card';
      stopAlarmInternal();
      alarmSilenced = false;
    }

    document.getElementById('relayToggle').checked = d.relay;
    document.getElementById('relayLabel').innerText = d.relay ? 'ON' : 'OFF';
    document.getElementById('thresholdInput').value = d.threshold;

    const switchWrap = document.getElementById('relaySwitchWrap');
    if (d.mode !== 'MANUAL') switchWrap.classList.add('disabled'); else switchWrap.classList.remove('disabled');

    history.push(d.temperature);
    if (history.length > maxPoints) history.shift();
    drawChart();
    addHistoryRow(d);

    document.getElementById('lastUpdate').innerText = new Date().toLocaleTimeString();
  } catch (e) {
    document.getElementById('dot').className = 'dot red';
  }
}

function drawChart() {
  const canvas = document.getElementById('chart');
  const ctx = canvas.getContext('2d');
  canvas.width = canvas.clientWidth;
  canvas.height = canvas.clientHeight;
  ctx.clearRect(0, 0, canvas.width, canvas.height);
  if (history.length < 2) return;

  const min = Math.min(...history) - 1;
  const max = Math.max(...history) + 1;
  const range = max - min || 1;
  const stepX = canvas.width / (maxPoints - 1);

  ctx.beginPath();
  ctx.strokeStyle = '#10b981';
  ctx.lineWidth = 2;
  history.forEach((val, i) => {
    const x = i * stepX;
    const y = canvas.height - ((val - min) / range) * canvas.height;
    if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
  });
  ctx.stroke();
}

async function toggleRelay() {
  const state = document.getElementById('relayToggle').checked ? 1 : 0;
  await fetch('/relay?state=' + state);
}

async function setMode(mode) {
  await fetch('/setmode?mode=' + mode);
  fetchData();
}

async function saveThreshold() {
  const value = document.getElementById('thresholdInput').value;
  await fetch('/threshold?value=' + value);
}

async function saveAiServerIp() {
  const ip = document.getElementById('aiIpInput').value;
  await fetch('/setip?ip=' + encodeURIComponent(ip));
  alert('AI Server IP updated to: ' + ip);
}

// ---------- Startup ----------
const savedSetup = JSON.parse(localStorage.getItem('farmSetup') || '{}');
if (savedSetup.crop && savedSetup.season) {
  document.getElementById('setupModal').style.display = 'none';
  updateCropSummary();
} else {
  document.getElementById('setupModal').style.display = 'flex';
}

setInterval(fetchData, 2000);
fetchData();
</script>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send_P(200, "text/html", htmlPage);
}

void handleData() {
  String json = "{";
  json += "\"temperature\":" + String(temperature, 1) + ",";
  json += "\"humidity\":" + String(humidity, 1) + ",";
  json += "\"light\":" + String(lightIntensity) + ",";
  json += "\"rain\":" + String(rainStatus == LOW ? "true" : "false") + ",";
  json += "\"soil\":" + String(soilValue) + ",";
  json += "\"water\":" + String(waterLevel) + ",";
  json += "\"relay\":" + String(relayOn ? "true" : "false") + ",";
  json += "\"mode\":\"" + currentMode + "\",";
  json += "\"threshold\":" + String(soilThreshold) + ",";
  json += "\"ai_ip\":\"" + String(aiServerIP) + "\",";
  json += "\"ai_connected\":" + String(aiServerConnected ? "true" : "false") + ",";
  json += "\"ai_action\":\"" + aiAction + "\",";
  json += "\"ai_water\":" + String(aiPredictedWater, 1) + ",";
  json += "\"ai_runtime\":" + String(aiRuntimeSec, 1) + ",";
  json += "\"ai_reason\":\"" + aiReason + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void handleRelay() {
  if (currentMode == "MANUAL" && server.hasArg("state")) {
    relayOn = server.arg("state") == "1";
    digitalWrite(UNO_SIGNAL_PIN, relayOn ? HIGH : LOW);
  }
  server.send(200, "text/plain", "OK");
}

void handleSetMode() {
  if (server.hasArg("mode")) {
    currentMode = server.arg("mode");
    if (currentMode == "THRESHOLD_AUTO") {
      relayOn = (soilValue > soilThreshold);
      digitalWrite(UNO_SIGNAL_PIN, relayOn ? HIGH : LOW);
    } else if (currentMode == "AI_AUTO") {
      relayOn = (aiAction == "PUMP_ON");
      digitalWrite(UNO_SIGNAL_PIN, relayOn ? HIGH : LOW);
    }
  }
  server.send(200, "text/plain", "OK");
}

void handleThreshold() {
  if (server.hasArg("value")) {
    soilThreshold = server.arg("value").toInt();
  }
  server.send(200, "text/plain", "OK");
}

void handleSetIp() {
  if (server.hasArg("ip")) {
    String ipStr = server.arg("ip");
    ipStr.toCharArray(aiServerIP, sizeof(aiServerIP));
    Serial.print("AI Server IP changed to: ");
    Serial.println(aiServerIP);
  }
  server.send(200, "text/plain", "OK");
}

void handleSetCrop() {
  if (server.hasArg("crop")) selectedCrop = server.arg("crop");
  if (server.hasArg("season")) selectedSeason = server.arg("season");
  server.send(200, "text/plain", "OK");
}

// Send Telemetry to Python AI FastAPI Server
void sendToAIServer() {
  if (WiFi.status() != WL_CONNECTED) {
    aiServerConnected = false;
    return;
  }

  HTTPClient http;
  String url = "http://" + String(aiServerIP) + ":" + String(aiServerPort) + "/api/telemetry";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(2500);

  String payload = "{";
  payload += "\"temperature\":" + String(temperature, 1) + ",";
  payload += "\"humidity\":" + String(humidity, 1) + ",";
  payload += "\"soil_value\":" + String(soilValue) + ",";
  payload += "\"light_value\":" + String(lightIntensity) + ",";
  payload += "\"water_level\":" + String(waterLevel) + ",";
  payload += "\"rain\":\"" + String(rainStatus == LOW ? "YES" : "NO") + "\",";
  payload += "\"soil_status\":\"" + String(soilValue > soilThreshold ? "DRY" : "WET") + "\",";
  payload += "\"tank_status\":\"" + String(waterLevel < 500 ? "EMPTY" : "AVAILABLE") + "\",";
  payload += "\"crop\":\"" + selectedCrop + "\",";
  payload += "\"season\":\"" + selectedSeason + "\",";
  payload += "\"growth_stage\":\"Vegetative\",";
  payload += "\"soil_type\":\"Loamy\",";
  payload += "\"pump_flow_lpm\":2.5";
  payload += "}";

  int httpCode = http.POST(payload);
  if (httpCode == 200) {
    aiServerConnected = true;
    String resp = http.getString();
    
    // Parse JSON Response fields manually
    int actIdx = resp.indexOf("\"action\":\"");
    if (actIdx != -1) {
      int endQuote = resp.indexOf("\"", actIdx + 10);
      if (endQuote != -1) aiAction = resp.substring(actIdx + 10, endQuote);
    }
    
    int waterIdx = resp.indexOf("\"predicted_water_required_l\":");
    if (waterIdx != -1) {
      int commaIdx = resp.indexOf(",", waterIdx);
      if (commaIdx == -1) commaIdx = resp.indexOf("}", waterIdx);
      if (commaIdx != -1) aiPredictedWater = resp.substring(waterIdx + 29, commaIdx).toFloat();
    }
    
    int runIdx = resp.indexOf("\"pump_runtime_sec\":");
    if (runIdx != -1) {
      int commaIdx = resp.indexOf(",", runIdx);
      if (commaIdx == -1) commaIdx = resp.indexOf("}", runIdx);
      if (commaIdx != -1) aiRuntimeSec = resp.substring(runIdx + 19, commaIdx).toFloat();
    }
    
    int reasonIdx = resp.indexOf("\"status_reason\":\"");
    if (reasonIdx != -1) {
      int endQuote = resp.indexOf("\"", reasonIdx + 17);
      if (endQuote != -1) aiReason = resp.substring(reasonIdx + 17, endQuote);
    }

    int modeIdx = resp.indexOf("\"mode\":\"");
    if (modeIdx != -1) {
      int endQuote = resp.indexOf("\"", modeIdx + 8);
      if (endQuote != -1) currentMode = resp.substring(modeIdx + 8, endQuote);
    }

    int targetIdx = resp.indexOf("\"target_pump_state\":");
    if (targetIdx != -1) {
      bool targetState = (resp.substring(targetIdx + 20, targetIdx + 24) == "true");
      if (currentMode == "AI_AUTO") {
        relayOn = targetState;
        digitalWrite(UNO_SIGNAL_PIN, relayOn ? HIGH : LOW);
      }
    }
  } else {
    aiServerConnected = false;
  }
  http.end();
}

// Background FreeRTOS task for Google Sheets
void sheetsTask(void* param) {
  for (;;) {
    sendToGoogleSheets();
    vTaskDelay(sheetSendInterval / portTICK_PERIOD_MS);
  }
}

void sendToGoogleSheets() {
  if (WiFi.status() != WL_CONNECTED) return;

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient https;
  String url = String(scriptURL) +
               "?temperature=" + String(temperature, 1) +
               "&humidity=" + String(humidity, 1) +
               "&light=" + String(lightIntensity) +
               "&rain=" + String(rainStatus == LOW ? "1" : "0") +
               "&soil=" + String(soilValue) +
               "&water=" + String(waterLevel) +
               "&pump=" + String(relayOn ? "1" : "0");

  if (https.begin(client, url)) {
    int httpCode = https.GET();
    Serial.print("Sheets log status: ");
    Serial.println(httpCode);
    https.end();
  } else {
    Serial.println("Unable to connect to Google Sheets URL");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("BOOT OK - Smart AIoT Irrigation");

  Wire.begin(SDA_PIN, SCL_PIN);
  lcd.init();
  lcd.backlight();
  dht.begin();

  pinMode(RAIN_PIN, INPUT);
  pinMode(UNO_SIGNAL_PIN, OUTPUT);
  digitalWrite(UNO_SIGNAL_PIN, LOW);

  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(500);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  Serial.println();
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("WiFi FAILED. Status code: ");
    Serial.println(WiFi.status());
  }
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("IP:");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
  delay(3000);
  lcd.clear();

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/relay", handleRelay);
  server.on("/setmode", handleSetMode);
  server.on("/threshold", handleThreshold);
  server.on("/setip", handleSetIp);
  server.on("/setcrop", handleSetCrop);
  server.begin();
  Serial.println("Web server started. Open IP in browser.");

  xTaskCreatePinnedToCore(sheetsTask, "SheetsTask", 16384, NULL, 1, NULL, 0);
}

void loop() {
  server.handleClient();
  unsigned long now = millis();

  // Read sensors
  if (now - lastRead >= readInterval) {
    lastRead = now;
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) {
      temperature = t;
      humidity = h;
    }
    lightIntensity = analogRead(LDR_PIN);
    rainStatus = digitalRead(RAIN_PIN);
    soilValue = analogRead(SOIL_PIN);
    waterLevel = analogRead(WATERLEVEL_PIN);

    // Traditional soil threshold mode fallback check
    if (currentMode == "THRESHOLD_AUTO") {
      if (soilValue > soilThreshold && rainStatus != LOW && waterLevel >= 500) {
        relayOn = true;
      } else {
        relayOn = false;
      }
      digitalWrite(UNO_SIGNAL_PIN, relayOn ? HIGH : LOW);
    }

    Serial.print("T:"); Serial.print(temperature);
    Serial.print(" H:"); Serial.print(humidity);
    Serial.print(" Soil:"); Serial.print(soilValue);
    Serial.print(" AI Water:"); Serial.print(aiPredictedWater);
    Serial.print(" Mode:"); Serial.print(currentMode);
    Serial.print(" Pump:"); Serial.println(relayOn ? "ON" : "OFF");
  }

  // Periodic AI Server Communication
  if (now - lastAIServerSync >= aiSyncInterval) {
    lastAIServerSync = now;
    sendToAIServer();
  }

  // LCD display cycler
  if (now - lastLcdSwitch >= 2500) {
    lastLcdSwitch = now;
    lcdScreen = (lcdScreen + 1) % 3;

    lcd.clear();

    if (lcdScreen == 0) {
      lcd.setCursor(0, 0);
      lcd.print("T:"); lcd.print(temperature, 1); lcd.print("C H:"); lcd.print(humidity, 0); lcd.print("%");
      lcd.setCursor(0, 1);
      lcd.print("Light:"); lcd.print(lightIntensity);
    } else if (lcdScreen == 1) {
      lcd.setCursor(0, 0);
      lcd.print("Soil:"); lcd.print(soilValue);
      lcd.setCursor(0, 1);
      lcd.print("AI H2O:"); lcd.print(aiPredictedWater, 1); lcd.print("L");
    } else {
      lcd.setCursor(0, 0);
      lcd.print("P:"); lcd.print(relayOn ? "ON " : "OFF");
      lcd.print(" "); lcd.print(currentMode.substring(0, 6));
      lcd.setCursor(0, 1);
      lcd.print(WiFi.localIP());
    }
  }
}