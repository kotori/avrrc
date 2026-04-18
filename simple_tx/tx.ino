/*
  AVRRC
  Tranmsitter code for the Arduino Radio control with PWM or PPM output via a
  nRF24 module.

      nRF Module -> Arduino UNO,NANO
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
#include <nRF24L01.h>

// --- FEATURE TOGGLES ---
#define USE_TRIM
#define USE_DEADZONE
#define DEBUG_MODE

// --- PINS ---
const int MIXER_SWITCH_PIN = 4;
const int BUTTON_CH5_PIN = 2;  // Calibration Trigger
const int BUTTON_CH6_PIN = 3;  // Bind Trigger

// --- EEPROM ADDRESSES ---
const int EE_ADDR_X1_MIN = 0;
const int EE_ADDR_X1_MAX = 2;
const int EE_ADDR_Y1_MIN = 4;
const int EE_ADDR_Y1_MAX = 6;
const int EE_ADDR_X2_MIN = 8;
const int EE_ADDR_X2_MAX = 10;
const int EE_ADDR_Y2_MIN = 12;
const int EE_ADDR_Y2_MAX = 14;

const int EE_ADDR_X1_CENTER = 16;
const int EE_ADDR_Y1_CENTER = 18;
const int EE_ADDR_X2_CENTER = 20;
const int EE_ADDR_Y2_CENTER = 22;

const int EE_ADDR_UNIQUE_ID = 100;  // 8-byte Master ID

// --- GLOBALS ---
int x1Min = 0, x1Max = 1023, y1Min = 0, y1Max = 1023;
int x2Min = 0, x2Max = 1023, y2Min = 0, y2Max = 1023;
int x1Center = 127, y1Center = 127, x2Center = 127, y2Center = 127;

uint64_t masterAddress;
const unsigned long txIntervalMillis = 20;
const float alpha = 0.2;
const int deadzoneRange = 4;
const byte NEUTRAL = 127;

unsigned long prevMillis = 0;
float smoothCh1, smoothCh2, smoothCh3, smoothCh4, smoothCh7;

// Our transmitter.
RF24 radio(9, 10);

struct __attribute__((packed)) Payload {
  byte channels[7];
} payload;

struct __attribute__((packed)) Telemetry {
  int rawVoltage;
  bool signalOk;
} telemetry;

// --- BINDING: Send ID to Receiver ---
void run_bind_mode() {
  Serial.println("BIND MODE: Broadcasting ID...");
  radio.stopListening();
  radio.openWritingPipe(0x0000000001LL);  // Global Bind Channel

  unsigned long start = millis();
  while (millis() - start < 5000) {  // Send for 5 seconds
    radio.write(&masterAddress, sizeof(masterAddress));
    digitalWrite(13, (millis() / 100) % 2);  // Rapid blink
    delay(50);
  }
  Serial.println("Bind Finished!");
  digitalWrite(13, LOW);
}

// --- CALIBRATION ROUTINES ---
void run_calibration() {
  Serial.println("CALIBRATION: Stir sticks fully for 5s!");
  unsigned long calStartTime = millis();
  x1Min = y1Min = x2Min = y2Min = 1023;
  x1Max = y1Max = x2Max = y2Max = 0;

  while (millis() - calStartTime < 5000) {
    int curX1 = analogRead(A0);
    int curY1 = analogRead(A1);
    int curX2 = analogRead(A2);
    int curY2 = analogRead(A3);

    if (curX1 < x1Min) x1Min = curX1;
    if (curX1 > x1Max) x1Max = curX1;
    if (curY1 < y1Min) y1Min = curY1;
    if (curY1 > y1Max) y1Max = curY1;
    if (curX2 < x2Min) x2Min = curX2;
    if (curX2 > x2Max) x2Max = curX2;
    if (curY2 < y2Min) y2Min = curY2;
    if (curY2 > y2Max) y2Max = curY2;

    digitalWrite(13, (millis() / 200) % 2);
  }

  Serial.println("Release sticks to CENTER...");
  delay(1000);
  long cx1 = 0, cy1 = 0, cx2 = 0, cy2 = 0;
  for (int i = 0; i < 50; i++) {
    cx1 += map(analogRead(A0), x1Min, x1Max, 0, 255);
    cy1 += map(analogRead(A1), y1Min, y1Max, 0, 255);
    cx2 += map(analogRead(A2), x2Min, x2Max, 0, 255);
    cy2 += map(analogRead(A3), y2Min, y2Max, 0, 255);
    delay(10);
  }
  x1Center = cx1 / 50;
  y1Center = cy1 / 50;
  x2Center = cx2 / 50;
  y2Center = cy2 / 50;

  EEPROM.put(EE_ADDR_X1_CENTER, x1Center);
  EEPROM.put(EE_ADDR_Y1_CENTER, y1Center);
  EEPROM.put(EE_ADDR_X2_CENTER, x2Center);
  EEPROM.put(EE_ADDR_Y2_CENTER, y2Center);
  EEPROM.put(EE_ADDR_X1_MIN, x1Min);
  EEPROM.put(EE_ADDR_X1_MAX, x1Max);
  EEPROM.put(EE_ADDR_Y1_MIN, y1Min);
  EEPROM.put(EE_ADDR_Y1_MAX, y1Max);
  EEPROM.put(EE_ADDR_X2_MIN, x2Min);
  EEPROM.put(EE_ADDR_X2_MAX, x2Max);
  EEPROM.put(EE_ADDR_Y2_MIN, y2Min);
  EEPROM.put(EE_ADDR_Y2_MAX, y2Max);
  Serial.println("Calibration Saved!");
}

// --- INPUT ROUTINES ---
void read_inputs() {
  int r1 = constrain(map(analogRead(A0), x1Min, x1Max, 0, 255), 0, 255);
  int r2 = constrain(map(analogRead(A1), y1Min, y1Max, 0, 255), 0, 255);
  int r3 = constrain(map(analogRead(A2), x2Min, x2Max, 0, 255), 0, 255);
  int r4 = constrain(map(analogRead(A3), y2Min, y2Max, 0, 255), 0, 255);
  int r7 = analogRead(A4) >> 2;

  smoothCh1 = (r1 * alpha) + (smoothCh1 * (1.0 - alpha));
  smoothCh2 = (r2 * alpha) + (smoothCh2 * (1.0 - alpha));
  smoothCh3 = (r3 * alpha) + (smoothCh3 * (1.0 - alpha));
  smoothCh4 = (r4 * alpha) + (smoothCh4 * (1.0 - alpha));
  smoothCh7 = (r7 * alpha) + (smoothCh7 * (1.0 - alpha));

#ifdef USE_DEADZONE
  if (abs((int)smoothCh1 - x1Center) < deadzoneRange) smoothCh1 = NEUTRAL;
  if (abs((int)smoothCh2 - y1Center) < deadzoneRange) smoothCh2 = NEUTRAL;
  if (abs((int)smoothCh3 - x2Center) < deadzoneRange) smoothCh3 = NEUTRAL;
  if (abs((int)smoothCh4 - y2Center) < deadzoneRange) smoothCh4 = NEUTRAL;
#endif

  if (digitalRead(MIXER_SWITCH_PIN) == LOW) {
    int throttle = (int)smoothCh2;
    int steering = (int)smoothCh1;
    payload.channels[0] = (byte)steering;
    payload.channels[1] =
        (byte)constrain(throttle + (steering - NEUTRAL), 0, 255);
    payload.channels[2] =
        (byte)constrain(throttle - (steering - NEUTRAL), 0, 255);
  } else {
    payload.channels[0] = (byte)smoothCh1;
    payload.channels[1] = (byte)smoothCh2;
    payload.channels[2] = (byte)smoothCh3;
  }
  payload.channels[3] = (byte)smoothCh4;
  payload.channels[6] = (byte)smoothCh7;
  payload.channels[4] = !digitalRead(BUTTON_CH5_PIN) ? 255 : 0;
  payload.channels[5] = !digitalRead(BUTTON_CH6_PIN) ? 255 : 0;
}

void setup() {
  Serial.begin(9600);
  pinMode(13, OUTPUT);
  pinMode(MIXER_SWITCH_PIN, INPUT_PULLUP);
  pinMode(BUTTON_CH5_PIN, INPUT_PULLUP);
  pinMode(BUTTON_CH6_PIN, INPUT_PULLUP);

  // 1. Load or Generate Unique ID
  EEPROM.get(EE_ADDR_UNIQUE_ID, masterAddress);
  if (masterAddress == 0 || masterAddress == 0xFFFFFFFFFFFFFFFFLL) {
    randomSeed(analogRead(A7));
    masterAddress = ((uint64_t)random() << 32) | random();
    EEPROM.put(EE_ADDR_UNIQUE_ID, masterAddress);
  }

  // 2. Initial Radio Setup
  radio.begin();
  radio.setDataRate(RF24_250KBPS);
  radio.enableAckPayload();
  radio.setRetries(0, 15);

  // 3. Handle Special Modes
  if (digitalRead(BUTTON_CH6_PIN) == LOW) run_bind_mode();
  if (digitalRead(BUTTON_CH5_PIN) == LOW) run_calibration();

  // 4. Final Radio Pipe
  radio.openWritingPipe(masterAddress);

  // 5. Load Calibration
  EEPROM.get(EE_ADDR_X1_MIN, x1Min);
  EEPROM.get(EE_ADDR_X1_MAX, x1Max);
  EEPROM.get(EE_ADDR_Y1_MIN, y1Min);
  EEPROM.get(EE_ADDR_Y1_MAX, y1Max);
  EEPROM.get(EE_ADDR_X2_MIN, x2Min);
  EEPROM.get(EE_ADDR_X2_MAX, x2Max);
  EEPROM.get(EE_ADDR_Y2_MIN, y2Min);
  EEPROM.get(EE_ADDR_Y2_MAX, y2Max);
  EEPROM.get(EE_ADDR_X1_CENTER, x1Center);
  EEPROM.get(EE_ADDR_Y1_CENTER, y1Center);
  EEPROM.get(EE_ADDR_X2_CENTER, x2Center);
  EEPROM.get(EE_ADDR_Y2_CENTER, y2Center);

  smoothCh1 = smoothCh2 = smoothCh3 = smoothCh4 = NEUTRAL;
}

void loop() {
  unsigned long now = millis();
  if (now - prevMillis >= txIntervalMillis) {
    prevMillis = now;
    read_inputs();
    if (radio.write(&payload, sizeof(Payload))) {
      if (radio.isAckPayloadAvailable()) {
        radio.read(&telemetry, sizeof(telemetry));
#ifdef DEBUG_MODE
        if (telemetry.signalOk) {
          float v = telemetry.rawVoltage * (5.0 / 1024.0) * 3.127;
          Serial.print("RX Voltage: ");
          Serial.println(v, 2);
        }
#endif
      }
    }
  }
}
