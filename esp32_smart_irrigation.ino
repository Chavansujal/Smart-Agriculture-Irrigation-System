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
char aiServerIP[64] = "10.185.32.62";  // Host PC IP address (can be reconfigured anytime via Web UI)
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

// ---------- Relay & Actuator Configuration ----------
// relayActiveLow: Set to 'false' for Active-HIGH signal to Arduino Uno (ESP32 outputs 3.3V HIGH on GPIO 11 for ON)
// Set to 'true' if an Active-LOW 5V Relay module is wired DIRECTLY to ESP32 GPIO 11
bool relayActiveLow = false; 

// Hysteresis & Minimum Dwell Time to prevent rapid pump ON/OFF cycling / relay chatter
unsigned long lastRelaySwitchTime = 0;
const unsigned long minRelayDwellMs = 15000; // 15 seconds minimum switching dwell time
const int SOIL_HYSTERESIS = 80;

// Startup Safety Guard: Keep pump strictly locked OFF during boot until sensors settle
const unsigned long STARTUP_GUARD_MS = 10000; // 10 seconds startup stabilization guard
bool startupGuardPassed = false;
int validSensorReadingsCount = 0;
const int REQUIRED_VALID_SAMPLES = 2;

// Non-blocking Network Collision Gate
volatile bool isNetworkBusy = false;

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

// Direct hardware pin control respecting configured polarity
void setPumpRelay(bool turnOn) {
  bool previousState = relayOn;
  relayOn = turnOn;
  int pinLevel = relayActiveLow ? (turnOn ? LOW : HIGH) : (turnOn ? HIGH : LOW);
  digitalWrite(UNO_SIGNAL_PIN, pinLevel);
  if (previousState != turnOn) {
    Serial.printf("[UNO-SIGNAL] GPIO %d set to %s (Polarity: %s) | Relay State: %s\n",
                  UNO_SIGNAL_PIN,
                  pinLevel == HIGH ? "HIGH (3.3V)" : "LOW (0V)",
                  relayActiveLow ? "Active-LOW" : "Active-HIGH",
                  turnOn ? "ACTIVE_ON" : "INACTIVE_OFF");
    // Standard UART command output for Arduino Uno Serial RX
    Serial.println(turnOn ? F("[PUMP_CMD] PUMP_ON") : F("[PUMP_CMD] PUMP_OFF"));
  }
}

// Enforces physical safety locks, startup guard, and anti-chatter dwell time before toggling
void applyTargetPumpState(bool targetState) {
  // CRITICAL 1: STARTUP STABILIZATION GUARD - Hold pump strictly OFF during boot
  if (!startupGuardPassed || millis() < STARTUP_GUARD_MS) {
    if (relayOn) {
      setPumpRelay(false);
    }
    return;
  }

  // CRITICAL 2: HARDWARE DRY-RUN & RAIN SAFETY - Immediate zero-delay trip
  bool emptyTank = (waterLevel < 500);
  bool rainDetected = (rainStatus == LOW);

  if (emptyTank || rainDetected) {
    if (relayOn) {
      setPumpRelay(false);
      lastRelaySwitchTime = millis();
      Serial.printf("[SAFETY-LOCKOUT] PUMP FORCED OFF! Reason: %s (WaterLevel: %d, RainStatus: %d)\n",
                    emptyTank ? "Tank Empty (<500)" : "Rain Detected",
                    waterLevel, rainStatus);
    }
    return;
  }

  // CRITICAL 3: Minimum switching dwell time before normal state transitions
  unsigned long now = millis();
  if (targetState != relayOn) {
    bool canSwitch = (lastRelaySwitchTime == 0 || now - lastRelaySwitchTime >= minRelayDwellMs);
    // In MANUAL mode, operator turn OFF request is executed immediately
    if (!targetState && currentMode == "MANUAL") {
      canSwitch = true;
    }

    if (canSwitch) {
      Serial.printf("[PUMP-CONTROL] State Transition: %s -> %s (Target: %s, Soil: %d, Mode: %s)\n",
                    relayOn ? "ON" : "OFF",
                    targetState ? "ON" : "OFF",
                    targetState ? "ON" : "OFF",
                    soilValue, currentMode.c_str());
      setPumpRelay(targetState);
      lastRelaySwitchTime = now;
    } else {
      Serial.printf("[ANTI-CHATTER] Dwell hold active (%lu ms remaining). Transition to %s deferred.\n",
                    minRelayDwellMs - (now - lastRelaySwitchTime),
                    targetState ? "ON" : "OFF");
    }
  }
}

// Auto irrigation threshold: Above 2500 = DRY (Pump ON), Below 2500 = WET (Pump OFF)
int soilThreshold = 2500;

unsigned long lastRead = 0;
const unsigned long readInterval = 2000;
unsigned long lastLcdSwitch = 0;
int lcdScreen = 0;

unsigned long lastSheetSend = 0;
const unsigned long sheetSendInterval = 15000;

// ---------- Web Dashboard HTML Page ----------
#include "webpage.h"

void handleRoot() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send_P(200, "text/html", htmlPage);
}

void handleData() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
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
  json += "\"ai_reason\":\"" + aiReason + "\",";
  json += "\"crop\":\"" + selectedCrop + "\",";
  json += "\"season\":\"" + selectedSeason + "\",";
  json += "\"polarity\":\"" + String(relayActiveLow ? "Active-LOW" : "Active-HIGH") + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void handleRelay() {
  if (currentMode == "MANUAL" && server.hasArg("state")) {
    bool req = (server.arg("state") == "1");
    Serial.printf("[WEB-MANUAL] Manual pump request: %s\n", req ? "ON" : "OFF");
    applyTargetPumpState(req);
  }
  server.send(200, "text/plain", "OK");
}

void handleSetMode() {
  if (server.hasArg("mode")) {
    currentMode = server.arg("mode");
    Serial.printf("[WEB-MODE] Mode changed to: %s\n", currentMode.c_str());
    if (currentMode == "THRESHOLD_AUTO") {
      bool shouldPump = (soilValue > soilThreshold);
      applyTargetPumpState(shouldPump);
    } else if (currentMode == "AI_AUTO") {
      bool shouldPump = (aiAction == "PUMP_ON");
      applyTargetPumpState(shouldPump);
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

void handlePolarity() {
  if (server.hasArg("active_low")) {
    relayActiveLow = (server.arg("active_low") == "1");
    int safeLevel = relayActiveLow ? (relayOn ? LOW : HIGH) : (relayOn ? HIGH : LOW);
    digitalWrite(UNO_SIGNAL_PIN, safeLevel);
    Serial.printf("[POLARITY] Active relay polarity switched to: %s\n", relayActiveLow ? "Active-LOW" : "Active-HIGH");
  }
  server.send(200, "text/plain", relayActiveLow ? "Active-LOW" : "Active-HIGH");
}

// ---------- Robust JSON Parsing Helper Functions for ESP32 ----------
bool parseBoolKey(const String& json, const String& key) {
  int idx = json.indexOf("\"" + key + "\":");
  if (idx == -1) return false;
  int start = idx + key.length() + 3;
  while (start < json.length() && (json[start] == ' ' || json[start] == '\t')) start++;
  return json.substring(start, start + 4).equalsIgnoreCase("true");
}

String parseStringKey(const String& json, const String& key) {
  String search = "\"" + key + "\":\"";
  int idx = json.indexOf(search);
  if (idx == -1) {
    search = "\"" + key + "\": \"";
    idx = json.indexOf(search);
    if (idx == -1) return "";
  }
  int start = idx + search.length();
  int endQuote = json.indexOf("\"", start);
  if (endQuote != -1) return json.substring(start, endQuote);
  return "";
}

float parseFloatKey(const String& json, const String& key) {
  int idx = json.indexOf("\"" + key + "\":");
  if (idx == -1) return 0.0;
  int start = idx + key.length() + 3;
  while (start < json.length() && (json[start] == ' ' || json[start] == '\t')) start++;
  int comma = json.indexOf(",", start);
  int brace = json.indexOf("}", start);
  int end = comma;
  if (end == -1 || (brace != -1 && brace < end)) end = brace;
  if (end != -1) return json.substring(start, end).toFloat();
  return 0.0;
}

int parseIntKey(const String& json, const String& key, int defaultVal) {
  int idx = json.indexOf("\"" + key + "\":");
  if (idx == -1) return defaultVal;
  int start = idx + key.length() + 3;
  while (start < json.length() && (json[start] == ' ' || json[start] == '\t')) start++;
  int comma = json.indexOf(",", start);
  int brace = json.indexOf("}", start);
  int end = comma;
  if (end == -1 || (brace != -1 && brace < end)) end = brace;
  if (end != -1) return json.substring(start, end).toInt();
  return defaultVal;
}

// Wi-Fi Connection Watchdog & Resiliency
unsigned long lastWiFiReconnectAttempt = 0;
const unsigned long wifiReconnectInterval = 5000;
int consecutiveSyncFailures = 0;

void maintainWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    aiServerConnected = false;
    unsigned long now = millis();
    if (now - lastWiFiReconnectAttempt >= wifiReconnectInterval) {
      lastWiFiReconnectAttempt = now;
      Serial.printf("[WIFI-WATCHDOG] Wi-Fi disconnected (status=%d)! Auto-reconnecting to '%s'...\n", WiFi.status(), ssid);
      WiFi.reconnect();
    }
  }
}

// Send Telemetry to Python AI FastAPI Server & Synchronize Hardware Control
void sendToAIServer() {
  if (WiFi.status() != WL_CONNECTED) {
    aiServerConnected = false;
    return;
  }
  if (isNetworkBusy) return;
  isNetworkBusy = true;

  HTTPClient http;
  String url = "http://" + String(aiServerIP) + ":" + String(aiServerPort) + "/api/telemetry";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(3500);

  String payload = "{";
  payload += "\"temperature\":" + String(temperature, 1) + ",";
  payload += "\"humidity\":" + String(humidity, 1) + ",";
  payload += "\"soil_value\":" + String(soilValue) + ",";
  payload += "\"light_value\":" + String(lightIntensity) + ",";
  payload += "\"water_level\":" + String(waterLevel) + ",";
  payload += "\"rain\":\"" + String(rainStatus == LOW ? "YES" : "NO") + "\",";
  payload += "\"soil_status\":\"" + String(soilValue > soilThreshold ? "DRY" : "WET") + "\",";
  payload += "\"tank_status\":\"" + String(waterLevel < 500 ? "EMPTY" : "AVAILABLE") + "\",";
  payload += "\"actual_pump_state\":" + String(relayOn ? "true" : "false") + ",";
  payload += "\"current_mode\":\"" + currentMode + "\",";
  payload += "\"crop\":\"" + selectedCrop + "\",";
  payload += "\"season\":\"" + selectedSeason + "\",";
  payload += "\"growth_stage\":\"Vegetative\",";
  payload += "\"soil_type\":\"Loamy\",";
  payload += "\"pump_flow_lpm\":2.5";
  payload += "}";

  int httpCode = http.POST(payload);
  if (httpCode == 200) {
    consecutiveSyncFailures = 0;
    aiServerConnected = true;
    String resp = http.getString();
    
    // Robust key-based parsing
    String parsedAction = parseStringKey(resp, "action");
    if (parsedAction.length() > 0) aiAction = parsedAction;

    aiPredictedWater = parseFloatKey(resp, "predicted_water_required_l");
    aiRuntimeSec = parseFloatKey(resp, "pump_runtime_sec");

    String parsedReason = parseStringKey(resp, "status_reason");
    if (parsedReason.length() > 0) aiReason = parsedReason;

    String parsedMode = parseStringKey(resp, "mode");
    if (parsedMode.length() > 0) currentMode = parsedMode;

    int newThreshold = parseIntKey(resp, "soil_threshold", -1);
    if (newThreshold > 0) soilThreshold = newThreshold;

    bool targetState = parseBoolKey(resp, "target_pump_state");
    bool desiredState = parseBoolKey(resp, "desired_pump_state");

    Serial.printf("[AI-SYNC-OK] Mode: %s | Target: %s | Desired: %s | ActualRelay: %s | Reason: %s\n",
                  currentMode.c_str(),
                  targetState ? "ON" : "OFF",
                  desiredState ? "ON" : "OFF",
                  relayOn ? "ON" : "OFF",
                  aiReason.c_str());

    // Apply state through startup-safe, anti-chatter, and polarity-aware handler
    if (currentMode == "AI_AUTO" || currentMode == "MANUAL") {
      applyTargetPumpState(targetState);
    } else if (currentMode == "THRESHOLD_AUTO") {
      bool shouldPump = false;
      if (relayOn) {
        shouldPump = (soilValue >= (soilThreshold - SOIL_HYSTERESIS));
      } else {
        shouldPump = (soilValue >= (soilThreshold + SOIL_HYSTERESIS));
      }
      applyTargetPumpState(shouldPump);
    }
  } else {
    consecutiveSyncFailures++;
    aiServerConnected = false;
    Serial.printf("[AI-SYNC-ERR] HTTP %d (Failure %d/3) to %s:%d. Reason: AI Server unreachable\n",
                  httpCode, consecutiveSyncFailures, aiServerIP, aiServerPort);

    // Fail-Safe: If in AI_AUTO mode and 3 consecutive syncs fail,
    // turn OFF relay immediately to avoid flooding / unmonitored pumping
    if (currentMode == "AI_AUTO" && consecutiveSyncFailures >= 3) {
      if (relayOn) {
        Serial.println("[FAIL-SAFE] 3 consecutive sync failures in AI_AUTO! Immediately turning OFF pump.");
        setPumpRelay(false);
      }
      aiReason = "FAIL-SAFE: AI Server Disconnected";
    } else if (currentMode == "THRESHOLD_AUTO") {
      // Local sensor fallback in threshold mode with hysteresis
      bool shouldPump = false;
      if (relayOn) {
        shouldPump = (soilValue >= (soilThreshold - SOIL_HYSTERESIS));
      } else {
        shouldPump = (soilValue >= (soilThreshold + SOIL_HYSTERESIS));
      }
      applyTargetPumpState(shouldPump);
    }
  }
  http.end();
  isNetworkBusy = false;
}

// Background FreeRTOS task for Google Sheets with non-blocking rate limiter
void sheetsTask(void* param) {
  for (;;) {
    vTaskDelay(sheetSendInterval / portTICK_PERIOD_MS);
    if (WiFi.status() == WL_CONNECTED && aiServerConnected && !isNetworkBusy) {
      sendToGoogleSheets();
    }
  }
}

void sendToGoogleSheets() {
  if (WiFi.status() != WL_CONNECTED || isNetworkBusy) return;
  isNetworkBusy = true;

  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(4);

  HTTPClient https;
  https.setTimeout(4000);
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
    Serial.printf("[SHEETS] Sync status: %d\n", httpCode);
    https.end();
  } else {
    Serial.println("[SHEETS] Unable to connect to Google Sheets URL");
  }
  isNetworkBusy = false;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("BOOT OK - Smart AIoT Irrigation");

  // CRITICAL: Firmly initialize relay pin to safe inactive level first
  pinMode(UNO_SIGNAL_PIN, OUTPUT);
  int safeLevel = relayActiveLow ? HIGH : LOW;
  digitalWrite(UNO_SIGNAL_PIN, safeLevel);
  relayOn = false;
  lastRelaySwitchTime = millis() + STARTUP_GUARD_MS;

  Wire.begin(SDA_PIN, SCL_PIN);
  lcd.init();
  lcd.backlight();
  dht.begin();

  pinMode(RAIN_PIN, INPUT);

  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");

  WiFi.mode(WIFI_STA);
  delay(100);
  WiFi.begin(ssid, password);
  WiFi.setAutoReconnect(true);
  WiFi.setSleep(false);
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
  } else {
    Serial.print("Connected! IP address: ");
    Serial.println(WiFi.localIP());
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("IP:");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
  delay(2000);
  lcd.clear();

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/relay", handleRelay);
  server.on("/setmode", handleSetMode);
  server.on("/threshold", handleThreshold);
  server.on("/setip", handleSetIp);
  server.on("/setcrop", handleSetCrop);
  server.on("/polarity", handlePolarity);
  server.begin();
  Serial.println("Web server started. Open IP in browser.");

  xTaskCreatePinnedToCore(sheetsTask, "SheetsTask", 16384, NULL, 1, NULL, 0);
}

void loop() {
  server.handleClient();
  maintainWiFi();
  unsigned long now = millis();

  // Evaluate Startup Guard completion
  if (!startupGuardPassed) {
    if (now >= STARTUP_GUARD_MS && validSensorReadingsCount >= REQUIRED_VALID_SAMPLES) {
      startupGuardPassed = true;
      lastRelaySwitchTime = now;
      Serial.printf("[STARTUP-GUARD] Normal operation enabled at t=%lu ms (Valid samples: %d)\n", now, validSensorReadingsCount);
    }
  }

  // Read sensors
  if (now - lastRead >= readInterval) {
    lastRead = now;
    // Read sensors with boundary validation
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t) && t >= -10.0 && t <= 70.0) {
      temperature = t;
    }
    if (!isnan(h) && h >= 0.0 && h <= 100.0) {
      humidity = h;
    }
    lightIntensity = constrain(analogRead(LDR_PIN), 0, 4095);
    rainStatus = digitalRead(RAIN_PIN);
    soilValue = constrain(analogRead(SOIL_PIN), 0, 4095);
    waterLevel = constrain(analogRead(WATERLEVEL_PIN), 0, 4095);

    if (waterLevel > 0 && soilValue > 0) {
      validSensorReadingsCount++;
    }

    // Continuous safety enforcement: if tank empty, rain detected, or in startup guard, trip off immediately
    if (waterLevel < 500 || rainStatus == LOW || !startupGuardPassed) {
      if (relayOn) {
        setPumpRelay(false);
      }
    } else if (currentMode == "THRESHOLD_AUTO") {
      bool shouldPump = false;
      if (relayOn) {
        shouldPump = (soilValue >= (soilThreshold - SOIL_HYSTERESIS));
      } else {
        shouldPump = (soilValue >= (soilThreshold + SOIL_HYSTERESIS));
      }
      applyTargetPumpState(shouldPump);
    }

    Serial.print("T:"); Serial.print(temperature);
    Serial.print(" H:"); Serial.print(humidity);
    Serial.print(" Soil:"); Serial.print(soilValue);
    Serial.print(" Water:"); Serial.print(waterLevel);
    Serial.print(" Mode:"); Serial.print(currentMode);
    Serial.print(" Startup:"); Serial.print(startupGuardPassed ? "PASS" : "GUARD");
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
      if (waterLevel < 500) {
        lcd.print("!EMPTY! Wtr:"); lcd.print(waterLevel);
      } else {
        lcd.print("Soil:"); lcd.print(soilValue); lcd.print(" W:"); lcd.print(waterLevel);
      }
      lcd.setCursor(0, 1);
      lcd.print("AI H2O:"); lcd.print(aiPredictedWater, 1); lcd.print("L");
    } else {
      lcd.setCursor(0, 0);
      lcd.print("P:"); lcd.print(relayOn ? "ON " : "OFF");
      lcd.print(" "); lcd.print(currentMode.substring(0, 4));
      lcd.print(aiServerConnected ? " AI:OK" : " AI:ERR");
      lcd.setCursor(0, 1);
      lcd.print(WiFi.localIP());
    }
  }
}