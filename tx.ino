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
const int BUTTON_CH5_PIN = 2;
const int BUTTON_CH6_PIN = 3;

// --- EEPROM ADDRESSES ---
const int EE_ADDR_X1_MIN = 0;
const int EE_ADDR_X1_MAX = 2;  // A0
const int EE_ADDR_Y1_MIN = 4;
const int EE_ADDR_Y1_MAX = 6;  // A1
const int EE_ADDR_X2_MIN = 8;
const int EE_ADDR_X2_MAX = 10;  // A2
const int EE_ADDR_Y2_MIN = 12;
const int EE_ADDR_Y2_MAX = 14;  // A3

int x1Min = 0, x1Max = 1023, y1Min = 0, y1Max = 1023;
int x2Min = 0, x2Max = 1023, y2Min = 0, y2Max = 1023;

// --- EEPROM ADDRESSES (Extended) ---
// Min/Max take up 0-15, Center points start at 16
const int EE_ADDR_X1_CENTER = 16;
const int EE_ADDR_Y1_CENTER = 18;
const int EE_ADDR_X2_CENTER = 20;
const int EE_ADDR_Y2_CENTER = 22;

int x1Center = 127, y1Center = 127, x2Center = 127, y2Center = 127;

// --- SETTINGS ---
const uint64_t radioPipe = 0xE8E8F0F0E1LL;
const unsigned long txIntervalMillis = 20;
const float alpha = 0.2;
const int deadzoneRange = 4;
const byte NEUTRAL = 127;

int xMin = 0, xMax = 1023, yMin = 0, yMax = 1023;
unsigned long prevMillis = 0;
float smoothCh1, smoothCh2, smoothCh3, smoothCh4, smoothCh7;

RF24 radio(9, 10);

struct __attribute__((packed)) Payload {
  byte channels[7];  // Array is much safer for loops than named members
} payload;

struct __attribute__((packed)) Telemetry {
  int rawVoltage;
  bool signalOk;
} telemetry;

void run_calibration() {
  Serial.println("CALIBRATION: Stir both sticks fully for 5s!");
  unsigned long calStartTime = millis();

  // Initialize with opposite extremes
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

    if (millis() % 200 < 100)
      digitalWrite(13, HIGH);
    else
      digitalWrite(13, LOW);
  }

  Serial.println("\nRelease sticks to CENTER now...");
  delay(1000);  // Give user time to let go

  // Average 50 readings for a rock-solid center point
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

  // Save everything to EEPROM
  EEPROM.put(EE_ADDR_X1_CENTER, x1Center);
  EEPROM.put(EE_ADDR_Y1_CENTER, y1Center);
  EEPROM.put(EE_ADDR_X2_CENTER, x2Center);
  EEPROM.put(EE_ADDR_Y2_CENTER, y2Center);
  Serial.println("Full Calibration Complete!");
}

void read_inputs() {
  // Map all four sticks using their unique calibrated ranges
  int raw1 = constrain(map(analogRead(A0), x1Min, x1Max, 0, 255), 0, 255);
  int raw2 = constrain(map(analogRead(A1), y1Min, y1Max, 0, 255), 0, 255);
  int raw3 = constrain(map(analogRead(A2), x2Min, x2Max, 0, 255), 0, 255);
  int raw4 = constrain(map(analogRead(A3), y2Min, y2Max, 0, 255), 0, 255);
  int raw7 = analogRead(A4) >> 2;  // Potentiometer

  // Apply alpha smoothing as before...
  smoothCh1 = (raw1 * alpha) + (smoothCh1 * (1.0 - alpha));
  smoothCh2 = (raw2 * alpha) + (smoothCh2 * (1.0 - alpha));
  smoothCh3 = (raw3 * alpha) + (smoothCh3 * (1.0 - alpha));
  smoothCh4 = (raw4 * alpha) + (smoothCh4 * (1.0 - alpha));
  smoothCh7 = (raw7 * alpha) + (smoothCh7 * (1.0 - alpha));

#ifdef USE_DEADZONE
  // If the smoothed value is within 'deadzoneRange' of its OWN calibrated
  // center:
  if (abs((int)smoothCh1 - x1Center) < deadzoneRange) smoothCh1 = NEUTRAL;
  if (abs((int)smoothCh2 - y1Center) < deadzoneRange) smoothCh2 = NEUTRAL;
  if (abs((int)smoothCh3 - x2Center) < deadzoneRange) smoothCh3 = NEUTRAL;
  if (abs((int)smoothCh4 - y2Center) < deadzoneRange) smoothCh4 = NEUTRAL;
#endif

  // Assign to payload array
  payload.channels[0] = (byte)smoothCh1;
  payload.channels[1] = (byte)smoothCh2;
  payload.channels[2] = (byte)smoothCh3;
  payload.channels[3] = (byte)smoothCh4;
  payload.channels[6] = (byte)smoothCh7;

  // Channels 5 & 6 (Buttons)
  payload.channels[4] = !digitalRead(BUTTON_CH5_PIN) ? 255 : 0;
  payload.channels[5] = !digitalRead(BUTTON_CH6_PIN) ? 255 : 0;
}

void setup() {
  Serial.begin(9600);
  radio.begin();
  radio.setDataRate(RF24_250KBPS);
  radio.enableAckPayload();  // Enable Telemetry
  radio.setRetries(0, 15);   // High-speed RC retries
  radio.openWritingPipe(radioPipe);

  pinMode(MIXER_SWITCH_PIN, INPUT_PULLUP);
  pinMode(BUTTON_CH5_PIN, INPUT_PULLUP);
  pinMode(BUTTON_CH6_PIN, INPUT_PULLUP);

  if (digitalRead(BUTTON_CH5_PIN) == LOW)
    run_calibration();
  else {
    EEPROM.get(EE_ADDR_X_MIN, xMin);
    EEPROM.get(EE_ADDR_X_MAX, xMax);
    EEPROM.get(EE_ADDR_Y_MIN, yMin);
    EEPROM.get(EE_ADDR_Y_MAX, yMax);
  }

  smoothCh1 = smoothCh2 = smoothCh3 = smoothCh4 = NEUTRAL;
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - prevMillis >= txIntervalMillis) {
    prevMillis = currentMillis;
    read_inputs();
    if (radio.write(&payload, sizeof(Payload))) {
      if (radio.isAckPayloadAvailable()) {
        radio.read(&telemetry, sizeof(telemetry));
#ifdef DEBUG_MODE
        if (telemetry.signalOk) {
          float voltage = telemetry.rawVoltage * (5.0 / 1024.0) * 3.127;
          Serial.print("RX Voltage: ");
          Serial.println(voltage, 2);
        }
#endif
      }
    }
  }
}
