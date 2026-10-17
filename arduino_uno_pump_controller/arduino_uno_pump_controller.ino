/*
 * ==============================================================================
 * Smart AIoT Irrigation - Arduino Uno Pump Controller Firmware
 * ==============================================================================
 * Architecture & Wiring:
 *   - ESP32-S3 GPIO 11 (UNO_SIGNAL_PIN) -> Arduino Uno Digital Pin 2
 *   - Arduino Uno GND                   -> ESP32-S3 GND (CRITICAL: COMMON GROUND!)
 *   - Arduino Uno Digital Pin 8         -> 5V Relay Module IN pin
 *   - Arduino Uno 5V & GND              -> 5V Relay Module VCC & GND
 *   - External Motor Power Supply       -> Switched through Relay COM & NO contacts
 * ==============================================================================
 * Interactive Serial Commands (115200 baud):
 *   - TEST      : Tests the active relay pin (clicks ON for 2s, then OFF)
 *   - SWEEP     : Automatically tests Pins 4, 7, 8, 12, 13 one by one to find your relay!
 *   - PIN <num> : Sets the relay control pin (e.g. "PIN 7", "PIN 8", "PIN 4")
 *   - POLARITY  : Toggles Active-LOW (default) vs Active-HIGH
 *   - ON / OFF  : Manually energizes or de-energizes the relay
 *   - STATUS    : Shows current pin levels, polarity, and uptime
 * ==============================================================================
 */

#define ESP_SIGNAL_PIN 2       // Digital input from ESP32-S3 GPIO 11
int relayOutputPin = 8;        // Active relay output pin (default: Pin 8)

// RELAY MODULE POLARITY CONFIGURATION:
// Set to 'true' if your relay turns ON when IN is LOW (95% of 5V Arduino relay modules are Active-LOW)
// Set to 'false' if your relay turns ON when IN is HIGH (Active-HIGH modules)
bool unoRelayActiveLow = true;

// Safety Guard Timings:
const unsigned long BOOT_SAFETY_LOCKOUT_MS = 3000;   // 3s boot guard against power-on transients
const unsigned long DEBOUNCE_DELAY_MS = 60;          // 60ms signal debounce to eliminate noise
const unsigned long MAX_CONTINUOUS_RUN_MS = 600000;  // 10 minutes max run safety watchdog

bool relayState = false;
int lastRawSignal = LOW;
unsigned long lastDebounceTime = 0;
int stableSignal = LOW;
unsigned long pumpStartTime = 0;
String serialCmdBuffer = "";

// Applies the electrical signal to the physical relay pin according to active polarity
void setRelay(bool energize, const char* source) {
  relayState = energize;
  int outputPinLevel = unoRelayActiveLow ? (energize ? LOW : HIGH) : (energize ? HIGH : LOW);
  
  pinMode(relayOutputPin, OUTPUT);
  digitalWrite(relayOutputPin, outputPinLevel);

  // Mirror state on Arduino Uno onboard LED (Pin 13) so you can physically see it!
  digitalWrite(LED_BUILTIN, energize ? HIGH : LOW);

  if (energize) {
    pumpStartTime = millis();
  }

  Serial.print(F("[UNO-ACTUATOR] Relay: "));
  Serial.print(energize ? F("ENERGIZED (PUMP ON)") : F("DE-ENERGIZED (PUMP OFF)"));
  Serial.print(F(" | Pin "));
  Serial.print(relayOutputPin);
  Serial.print(F(" output: "));
  Serial.print(outputPinLevel == HIGH ? F("HIGH (5V)") : F("LOW (0V)"));
  Serial.print(F(" | Onboard LED (Pin 13): "));
  Serial.print(energize ? F("ON") : F("OFF"));
  Serial.print(F(" | Polarity: "));
  Serial.print(unoRelayActiveLow ? F("Active-LOW") : F("Active-HIGH"));
  Serial.print(F(" | Trigger: "));
  Serial.println(source);
}

void printStatus() {
  int dRead = digitalRead(ESP_SIGNAL_PIN);

  Serial.println(F("\n========================================"));
  Serial.println(F("       ARDUINO UNO PUMP CONTROLLER      "));
  Serial.println(F("========================================"));
  Serial.print(F("Relay State    : ")); Serial.println(relayState ? F("ACTIVE (ON)") : F("INACTIVE (OFF)"));
  Serial.print(F("Active Output  : Pin ")); Serial.print(relayOutputPin);
  Serial.print(F(" = ")); Serial.println(digitalRead(relayOutputPin) == HIGH ? F("HIGH (5V)") : F("LOW (0V)"));
  Serial.print(F("Relay Polarity : ")); Serial.println(unoRelayActiveLow ? F("Active-LOW (LOW=ON)") : F("Active-HIGH (HIGH=ON)"));
  Serial.print(F("Digital Pin 2  : ")); Serial.println(dRead == HIGH ? F("HIGH (3.3V/5V Detected from ESP32)") : F("LOW (0V - Off)"));
  Serial.print(F("Onboard LED 13 : ")); Serial.println(digitalRead(LED_BUILTIN) == HIGH ? F("ON") : F("OFF"));
  Serial.print(F("Uptime         : ")); Serial.print(millis() / 1000); Serial.println(F(" seconds"));
  if (relayState) {
    Serial.print(F("Run Duration   : ")); Serial.print((millis() - pumpStartTime) / 1000); Serial.println(F(" s / 600 s max"));
  }
  Serial.println(F("========================================\n"));
}

void runPinSweep() {
  int testPins[] = {8, 7, 4, 12, 13};
  int numPins = sizeof(testPins) / sizeof(testPins[0]);

  Serial.println(F("\n>>> STARTING RELAY PIN SWEEP <<<"));
  Serial.println(F("Testing Pins [8, 7, 4, 12, 13] for 2 seconds each."));
  Serial.println(F("Listen for the relay CLICK and watch for the Relay LED!\n"));

  for (int i = 0; i < numPins; i++) {
    int pin = testPins[i];
    Serial.print(F("--> Testing Pin "));
    Serial.print(pin);
    Serial.println(F(" [PULSING LOW (0V) for Active-LOW relay]..."));

    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW); // Active-LOW ON
    digitalWrite(LED_BUILTIN, HIGH);
    delay(1500);

    Serial.print(F("    Pin "));
    Serial.print(pin);
    Serial.println(F(" [PULSING HIGH (5V) for Active-HIGH relay]..."));
    digitalWrite(pin, HIGH); // Active-HIGH ON
    delay(1500);

    // Return to safe high
    digitalWrite(pin, HIGH);
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
  }

  // Restore configured relay pin
  pinMode(relayOutputPin, OUTPUT);
  int safeLevel = unoRelayActiveLow ? HIGH : LOW;
  digitalWrite(relayOutputPin, safeLevel);

  Serial.println(F("\n>>> SWEEP COMPLETE <<<"));
  Serial.print(F("If your relay clicked on a specific pin, type 'PIN <number>' (e.g. 'PIN 7') to select it.\n"));
  Serial.print(F("Currently active pin is: Pin "));
  Serial.println(relayOutputPin);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println(F("\n[BOOT] Arduino Uno Pump Controller Starting..."));

  // 1. Immediately drive relay pin to SAFE INACTIVE level
  pinMode(relayOutputPin, OUTPUT);
  int safeLevel = unoRelayActiveLow ? HIGH : LOW;
  digitalWrite(relayOutputPin, safeLevel);
  relayState = false;

  // Onboard LED
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // 2. Configure input pin from ESP32
  pinMode(ESP_SIGNAL_PIN, INPUT);

  Serial.print(F("[BOOT] Safe inactive level written to Pin "));
  Serial.print(relayOutputPin);
  Serial.print(F(": "));
  Serial.println(safeLevel == HIGH ? F("HIGH") : F("LOW"));
  Serial.println(F("[BOOT] Boot safety guard active (3s). Pump locked OFF."));
  delay(BOOT_SAFETY_LOCKOUT_MS);

  Serial.println(F("[BOOT] Controller READY. Listening on Pin 2 and Serial (115200 baud)."));
  Serial.println(F("[CMD-HELP] Available Commands: TEST, SWEEP, PIN <num>, POLARITY, ON, OFF, STATUS"));
}

void loop() {
  // --- 1. Serial UART Command Processing (from PC Serial Monitor or ESP32) ---
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      serialCmdBuffer.trim();
      if (serialCmdBuffer.length() > 0) {
        String upperCmd = serialCmdBuffer;
        upperCmd.toUpperCase();
        Serial.print(F("[UNO-UART] Received command: '"));
        Serial.print(serialCmdBuffer);
        Serial.println(F("'"));

        if (upperCmd == "TEST") {
          Serial.println(F("[UNO-TEST] Running 2-second safe relay test click..."));
          Serial.println(F("[UNO-TEST] Watch the Arduino Uno onboard 'L' LED and listen for relay click!"));
          setRelay(true, "MANUAL_TEST");
          delay(2000);
          setRelay(false, "MANUAL_TEST_COMPLETE");
          Serial.println(F("[UNO-TEST] Safe test completed."));
        } else if (upperCmd == "SWEEP") {
          runPinSweep();
        } else if (upperCmd.startsWith("PIN ")) {
          int newPin = upperCmd.substring(4).toInt();
          if (newPin >= 2 && newPin <= 13) {
            // Restore old pin
            digitalWrite(relayOutputPin, unoRelayActiveLow ? HIGH : LOW);
            relayOutputPin = newPin;
            pinMode(relayOutputPin, OUTPUT);
            digitalWrite(relayOutputPin, unoRelayActiveLow ? HIGH : LOW);
            Serial.print(F("[UNO-CONFIG] Relay control pin changed to: Pin "));
            Serial.println(relayOutputPin);
          } else {
            Serial.println(F("[UNO-CONFIG] Invalid pin number. Use 2 to 13."));
          }
        } else if (upperCmd == "ON" || upperCmd == "PUMP_ON" || upperCmd == "PUMP:ON" || upperCmd == "1" || upperCmd.indexOf("PUMP_ON") != -1) {
          setRelay(true, "SERIAL_COMMAND_ON");
        } else if (upperCmd == "OFF" || upperCmd == "PUMP_OFF" || upperCmd == "PUMP:OFF" || upperCmd == "0" || upperCmd.indexOf("PUMP_OFF") != -1) {
          setRelay(false, "SERIAL_COMMAND_OFF");
        } else if (upperCmd == "STATUS") {
          printStatus();
        } else if (upperCmd == "POLARITY") {
          unoRelayActiveLow = !unoRelayActiveLow;
          int safeLevel = unoRelayActiveLow ? (relayState ? LOW : HIGH) : (relayState ? HIGH : LOW);
          digitalWrite(relayOutputPin, safeLevel);
          Serial.print(F("[UNO-CONFIG] Polarity switched to: "));
          Serial.println(unoRelayActiveLow ? F("Active-LOW") : F("Active-HIGH"));
        } else {
          Serial.print(F("[UNO-UART] Unknown command: "));
          Serial.println(serialCmdBuffer);
          Serial.println(F("[CMD-HELP] Try: TEST, SWEEP, PIN 8, POLARITY, ON, OFF, STATUS"));
        }
        serialCmdBuffer = "";
      }
    } else {
      if (serialCmdBuffer.length() < 32) {
        serialCmdBuffer += c;
      }
    }
  }

  // --- 2. Hardware Signal Reading & Debounce Filter (Digital Pin 2 from ESP32 GPIO 11) ---
  int rawRead = digitalRead(ESP_SIGNAL_PIN);

  if (rawRead != lastRawSignal) {
    lastDebounceTime = millis();
  }
  lastRawSignal = rawRead;

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY_MS) {
    if (rawRead != stableSignal) {
      stableSignal = rawRead;
      Serial.print(F("[UNO-SIGNAL] Stable ESP32 Input: "));
      Serial.println(stableSignal == HIGH ? F("HIGH (Request ON)") : F("LOW (Request OFF)"));

      if (stableSignal == HIGH && !relayState) {
        setRelay(true, "HARDWARE_PIN_SIGNAL");
      } else if (stableSignal == LOW && relayState) {
        setRelay(false, "HARDWARE_PIN_SIGNAL");
      }
    }
  }

  // --- 3. Safety Watchdog: Prevent continuous pumping > 10 mins ---
  if (relayState && (millis() - pumpStartTime >= MAX_CONTINUOUS_RUN_MS)) {
    Serial.println(F("[UNO-SAFETY-TRIP] Max continuous runtime exceeded (10 min)! Shutting off pump."));
    setRelay(false, "SAFETY_WATCHDOG_TIMEOUT");
  }
}
