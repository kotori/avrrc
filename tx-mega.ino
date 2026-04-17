/*
  AVRRC
  Tranmsitter code for the Arduino Radio control with PWM or PPM output via a
  nRF24 module.

      nRF Module -> Arduino Mega Pro Mini
      ==============================
        GND    ->   GND
        Vcc    ->   3.3V
        CE     ->   D9
        CSN    ->   D10
        CLK    ->   D13
        MOSI   ->   D11
        MISO   ->   D12

      Default Channel Mappings
      ========================
        1   ->    A0 [left joy x-axis]
        2   ->    A1 [left joy y-axis]
        3   ->    A2 [right joy x-axis]
        4   ->    A3 [right joy y-axis]
        5   ->    D2 [switch/pot input 1]
        6   ->    D3 [switch/pot input 2]
        7   ->    A4 [switch/pot input 3]
*/
#include <EEPROM.h>
#include <RF24.h>
#include <SPI.h>
#include <U8g2lib.h>
#include <nRF24L01.h>

// --- PACKED STRUCTURES (Ensure bit-perfect sync with Linux/RX) ---
struct __attribute__((packed)) ModelSettings {
  char name[12];
  uint64_t boatAddress;
  int xMin, xMax, yMin, yMax;
  int trims[4];
} currentModel;

struct __attribute__((packed)) Payload {
  byte channels[7]; // Array is much safer for loops than named members
};

struct __attribute__((packed)) Telemetry {
  float voltage;
  bool signalOk;
} telemetry;

// --- PINS (Advanced Mega Config) ---
const int BUTTON_A_PIN = 2;
const int BUTTON_B_PIN = 3;
const int MIXER_PIN = 4;
const int BUZZER_PIN = 5;
const int TRIM_PLUS = 6;
const int TRIM_MINUS = 7;
const int TRIM_SELECT = 8;
const int TX_BATT_PIN = A15;

// --- GLOBALS ---
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
RF24 radio(9, 10);

int activeIndex = 0;
int selectedTrimChannel = 0;
int linkQuality = 0;
unsigned long prevMillis = 0, prevLcdMillis = 0, lastTrimPress = 0;
float smoothCh1 = 127.0, smoothCh2 = 127.0;  // Smoothing filters
bool trimChanged = false;
bool throttleLocked = true;  // Safety lock active at boot

// --- HELPERS ---
void loadModel(int idx) {
  EEPROM.get(idx * sizeof(ModelSettings), currentModel);
}
void saveModel(int idx) {
  EEPROM.put(idx * sizeof(ModelSettings), currentModel);
}

// --- BINDING MODE ---
void handle_binding() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(10, 20, "!!! BIND MODE !!!");
  u8g2.setCursor(10, 40);
  u8g2.print("Model: ");
  u8g2.print(currentModel.name);
  u8g2.sendBuffer();

  radio.stopListening();
  radio.openWritingPipe(0x0000000001LL);

  unsigned long start = millis();
  while (millis() - start < 10000) {
    radio.write(&currentModel.boatAddress, sizeof(currentModel.boatAddress));
    tone(BUZZER_PIN, 2000, 50);
    delay(250);
  }
  tone(BUZZER_PIN, 1500, 500);
}

// --- SAFETY HANDLING ---
void handle_safety() {
  if (throttleLocked) {
    // A1 < 100 means the Left Y stick is pulled all the way down
    if (analogRead(A1) < 100 && digitalRead(BUTTON_A_PIN) == LOW) {
      throttleLocked = false;
      tone(BUZZER_PIN, 2000, 200);  // Happy beep for "Armed"
    }
  }
}

// --- LINUX SYNC PROTOCOL ---
void handle_pc_sync() {
  if (Serial.available() < 1) return;
  char cmd = Serial.read();
  if (cmd == 'G') {
    for (int i = 0; i < 20; i++) {
      ModelSettings m;
      EEPROM.get(i * sizeof(ModelSettings), m);
      Serial.write((byte*)&m, sizeof(ModelSettings));
    }
  }
  if (cmd == 'S') {
    tone(BUZZER_PIN, 1000, 500);
    for (int i = 0; i < 20; i++) {
      ModelSettings m;
      unsigned long start = millis();
      while (Serial.available() < sizeof(ModelSettings)) {
        if (millis() - start > 2000) return;
      }
      Serial.readBytes((char*)&m, sizeof(ModelSettings));
      EEPROM.put(i * sizeof(ModelSettings), m);
    }
    loadModel(activeIndex);
    tone(BUZZER_PIN, 2000, 200);
  }
}

void calibrate() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(10, 20, "STIR STICKS NOW...");
  u8g2.sendBuffer();
  currentModel.xMin = currentModel.yMin = 1023;
  currentModel.xMax = currentModel.yMax = 0;
  unsigned long start = millis();
  while (millis() - start < 5000) {
    int rx = analogRead(A0);
    int ry = analogRead(A1);
    if (rx < currentModel.xMin) currentModel.xMin = rx;
    if (rx > currentModel.xMax) currentModel.xMax = rx;
    if (ry < currentModel.yMin) currentModel.yMin = ry;
    if (ry > currentModel.yMax) currentModel.yMax = ry;
    u8g2.drawFrame(10, 40, 108, 10);
    u8g2.drawBox(10, 40, map(millis() - start, 0, 5000, 0, 108), 10);
    u8g2.sendBuffer();
  }
  saveModel(activeIndex);
  tone(BUZZER_PIN, 1500, 500);
}

void handle_stealth_trims() {
  unsigned long now = millis();
  if (digitalRead(TRIM_SELECT) == LOW && (now - lastTrimPress > 300)) {
    if (trimChanged) {
      saveModel(activeIndex);
      trimChanged = false;
    }
    selectedTrimChannel = (selectedTrimChannel + 1) % 4;
    tone(BUZZER_PIN, 1200, 50);
    lastTrimPress = now;
  }
  if (now - lastTrimPress > 150) {
    int dir = 0;
    if (digitalRead(TRIM_PLUS) == LOW)
      dir = 1;
    else if (digitalRead(TRIM_MINUS) == LOW)
      dir = -1;
    if (dir != 0) {
      int old = currentModel.trims[selectedTrimChannel];
      currentModel.trims[selectedTrimChannel] = constrain(old + dir, -40, 40);
      if (currentModel.trims[selectedTrimChannel] == 0 && old != 0)
        tone(BUZZER_PIN, 1800, 150);
      else
        tone(BUZZER_PIN, 1800, 20);
      trimChanged = true;
      lastTrimPress = now;
    }
  }
  if (trimChanged && (now - lastTrimPress > 5000)) {
    saveModel(activeIndex);
    trimChanged = false;
    tone(BUZZER_PIN, 800, 100);
  }
}

void read_inputs() {
  // --- 1. CAPTURE INPUTS ---
  int rawSticks[4] = {analogRead(A0), analogRead(A1), analogRead(A2),
                      analogRead(A3)};
  int processed[4];
  int out[4];  // Intermediate virtual channels

  // Map raw sensor data to a standard 0-255 scale
  for (int i = 0; i < 4; i++) {
    int mi = (i == 0) ? currentModel.xMin : (i == 1) ? currentModel.yMin : 0;
    int ma = (i == 0) ? currentModel.xMax : (i == 1) ? currentModel.yMax : 1023;
    processed[i] = map(rawSticks[i], mi, ma, 0, 255);
  }

  // Filter jitter on primary control sticks
  smoothCh1 = (smoothCh1 * 0.8) + (processed[0] * 0.2);
  smoothCh2 = (smoothCh2 * 0.8) + (processed[1] * 0.2);

  // --- 2. MIXER LOGIC (Virtual Mapping) ---
  bool mixerOn = (digitalRead(MIXER_PIN) == LOW);

  // The entire Right Stick can be used for a camera gimbal (Pan on Ch3, Tilt on
  // Ch4) when you aren't in Tank Mode.
  if (mixerOn) {
    // TANK MODE: Left Stick (A0/A1) drives both motors
    int steering = (int)smoothCh1 - 127;  // A0
    int throttle = (int)smoothCh2 - 127;  // A1
    if (abs(steering) < 5) steering = 0;

    out[0] = (int)smoothCh1;             // Ch1: Physical Rudder
    out[1] = 127 + throttle + steering;  // Ch2: Left Motor ESC
    out[2] = 127 + throttle - steering;  // Ch3: Right Motor ESC
    out[3] = processed[3];               // Ch4: Aux (Right Stick Y)
  } else {
    // STANDARD MODE: Left Stick (A1) is Throttle, Right Stick (A2) is Aux
    out[0] = (int)smoothCh1;  // Ch1: Physical Rudder (Left Stick X)
    out[1] = (int)smoothCh2;  // Ch2: Main Throttle (Left Stick Y)
    out[2] = processed[2];    // Ch3: Aux 1 (Right Stick X)
    out[3] = processed[3];    // Ch4: Aux 2 (Right Stick Y)
  }

  // --- 3. FINAL OUTPUTS (Trims & Constrain) ---
  
  // Apply safety lock
  if (throttleLocked) {
    out[1] = 127;
    out[2] = 127;
  }

  // Map to the Payload Array and apply trims
  for (int i = 0; i < 4; i++) {
    payload.channels[i] = (byte)constrain(out[i] + currentModel.trims[i], 0, 255);
  }

  // Buttons and Status
  payload.channels[4] = !digitalRead(BUTTON_A_PIN) ? 255 : 0;
  payload.channels[5] = !digitalRead(BUTTON_B_PIN) ? 255 : 0;
  payload.channels[6] = mixerOn ? 255 : 0;

  // --- 4. RADIO TRANSMISSION ---
  if (radio.write(&payload, sizeof(Payload))) {
    linkQuality = map(radio.getARC(), 0, 15, 100, 0);
    if (radio.isAckPayloadAvailable()) {
      radio.read(&telemetry, sizeof(telemetry));
    }
  } else {
    linkQuality = 0;
  }
}

void handle_battery_alarm() {
  static unsigned long lastAlarm = 0;
  static bool flash = false;
  if (telemetry.voltage > 1.0 && telemetry.voltage < 6.6) {
    if (millis() - lastAlarm > 1000) {
      lastAlarm = millis();
      flash = !flash;
      if (flash) {
        tone(BUZZER_PIN, 500, 200);
        u8g2.setContrast(0);
      } else
        u8g2.setContrast(255);
    }
  } else {
    u8g2.setContrast(255);
  }
}

void updateDisplay() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);

  // Header: Model Name & Link Quality
  u8g2.setCursor(0, 10);
  u8g2.print("MOD: ");
  u8g2.print(currentModel.name);
  u8g2.setCursor(95, 10);
  u8g2.print(linkQuality);
  u8g2.print("%");
  u8g2.drawLine(0, 13, 128, 13);

  // Center Row: Telemetry or Safety Warning
  if (throttleLocked) {
    u8g2.setCursor(30, 32);
    u8g2.print("! THROTTLE LOCKED !");
  } else {
    // Show RX Voltage
    if (telemetry.signalOk && linkQuality > 0) {
      u8g2.setCursor(0, 32);
      u8g2.print("RX: ");
      u8g2.print(telemetry.voltage, 1);
      u8g2.print("V");
    } else {
      u8g2.setCursor(0, 32);
      u8g2.print("--- NO LINK ---");
    }
    // Show TX Voltage
    float txV = analogRead(TX_BATT_PIN) * (5.0 / 1024.0) * 2.0;
    u8g2.setCursor(85, 32);
    u8g2.print("TX:");
    u8g2.print(txV, 1);
    u8g2.print("V");
  }

  // Footer: Trim Menu
  u8g2.drawFrame(0, 40, 128, 24);
  u8g2.setCursor(5, 52);
  u8g2.print("TRIM EDIT: CH");
  u8g2.print(selectedTrimChannel + 1);
  u8g2.setCursor(5, 62);
  u8g2.print("VALUE: ");
  u8g2.print(currentModel.trims[selectedTrimChannel] > 0 ? "+" : "");
  u8g2.print(currentModel.trims[selectedTrimChannel]);

  u8g2.sendBuffer();
}

void setup() {
  Serial.begin(9600);
  u8g2.begin();
  pinMode(BUTTON_A_PIN, INPUT_PULLUP);
  pinMode(BUTTON_B_PIN, INPUT_PULLUP);
  pinMode(MIXER_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(TRIM_PLUS, INPUT_PULLUP);
  pinMode(TRIM_MINUS, INPUT_PULLUP);
  pinMode(TRIM_SELECT, INPUT_PULLUP);

  EEPROM.get(1000, activeIndex);
  if (activeIndex < 0 || activeIndex >= 20) {
    activeIndex = 0;
    EEPROM.put(1000, activeIndex);
  }

  if (digitalRead(BUTTON_B_PIN) == LOW && digitalRead(BUTTON_A_PIN) == HIGH) {
    unsigned long selectStart = millis();
    while (millis() - selectStart < 5000) {
      if (digitalRead(BUTTON_B_PIN) == LOW) {
        activeIndex = (activeIndex + 1) % 20;
        loadModel(activeIndex);
        u8g2.clearBuffer();
        u8g2.drawStr(10, 20, "MODEL SELECT:");
        u8g2.setCursor(10, 45);
        u8g2.print("[");
        u8g2.print(activeIndex);
        u8g2.print("] ");
        u8g2.print(currentModel.name);
        u8g2.sendBuffer();
        tone(BUZZER_PIN, 1000, 50);
        delay(350);
        selectStart = millis();
      }
    }
    EEPROM.put(1000, activeIndex);
  }

  loadModel(activeIndex);

  if (currentModel.name[0] < 32 || currentModel.name[0] > 126) {
    strcpy(currentModel.name, "NEW MODEL");
    currentModel.boatAddress = 0xE8E8F0F0E1LL + activeIndex;
    currentModel.xMin = 0;
    currentModel.xMax = 1023;
    currentModel.yMin = 0;
    currentModel.yMax = 1023;
    for (int i = 0; i < 4; i++) currentModel.trims[i] = 0;
    saveModel(activeIndex);
  }

  if (digitalRead(BUTTON_A_PIN) == LOW && digitalRead(BUTTON_B_PIN) == LOW)
    handle_binding();
  if (digitalRead(BUTTON_A_PIN) == LOW && digitalRead(BUTTON_B_PIN) == HIGH)
    calibrate();

  radio.begin();
  radio.setDataRate(RF24_250KBPS);
  radio.enableAckPayload();
  radio.openWritingPipe(currentModel.boatAddress);
  radio.stopListening();
}

void loop() {
  unsigned long now = millis();

  handle_safety();         // 1. Check for unlock handshake.
  handle_stealth_trims();  // 2. Non-blocking menu.

  if (now - prevMillis >= 20) {
    prevMillis = now;
    read_inputs();  // 3. Process sticks and radio (locked/live)
  }

  if (now - prevLcdMillis >= 200) {
    prevLcdMillis = now;
    handle_pc_sync();        // 4. Check for linux companion commands.
    handle_battery_alarm();  // 5. Flash/Beep if RX voltage is LOW
    updateDisplay();         // 6. Draw the dashboard
  }
}
