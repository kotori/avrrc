/* 
  AVRRC
  Tranmsitter code for the Arduino Radio control with PWM or PPM output via a nRF24 module.
 
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

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <EEPROM.h>

// --- FEATURE TOGGLES ---
#define USE_TRIM
#define USE_DEADZONE
#define DEBUG_MODE

// --- PINS ---
const int MIXER_SWITCH_PIN = 4;
const int BUTTON_CH5_PIN = 2;
const int BUTTON_CH6_PIN = 3;

// --- EEPROM ADDRESSES ---
const int EE_ADDR_X_MIN = 0; const int EE_ADDR_X_MAX = 2;
const int EE_ADDR_Y_MIN = 4; const int EE_ADDR_Y_MAX = 6;

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
  byte channels[7]; // Array is much safer for loops than named members
} payload;

struct __attribute__((packed)) Telemetry {
  int rawVoltage;
  bool signalOk;
} telemetry;

void run_calibration() {
  Serial.println("CALIBRATION MODE: Move sticks now!");
  unsigned long calStartTime = millis();
  xMin = yMin = 1023; xMax = yMax = 0;
  while (millis() - calStartTime < 5000) {
    int curX = analogRead(A0); int curY = analogRead(A1);
    if (curX < xMin) xMin = curX; if (curX > xMax) xMax = curX;
    if (curY < yMin) yMin = curY; if (curY > yMax) yMax = curY;
    if (millis() % 200 < 100) Serial.print(".");
  }
  EEPROM.put(EE_ADDR_X_MIN, xMin); EEPROM.put(EE_ADDR_X_MAX, xMax);
  EEPROM.put(EE_ADDR_Y_MIN, yMin); EEPROM.put(EE_ADDR_Y_MAX, yMax);
  Serial.println("\nSaved!");
}

void read_inputs() {
  // 1. Process Raw Stick Data (Using 0-255 range)
  int raw1 = constrain(map(analogRead(A0), xMin, xMax, 0, 255), 0, 255);
  int raw2 = constrain(map(analogRead(A1), yMin, yMax, 0, 255), 0, 255);
  int raw3 = analogRead(A2) >> 2; // Right Joy X
  int raw4 = analogRead(A3) >> 2; // Right Joy Y
  int raw7 = analogRead(A4) >> 2; // Pot/Switch

  // 2. Apply Smoothing (Alpha Filter)
  smoothCh1 = (raw1 * alpha) + (smoothCh1 * (1.0 - alpha));
  smoothCh2 = (raw2 * alpha) + (smoothCh2 * (1.0 - alpha));
  smoothCh3 = (raw3 * alpha) + (smoothCh3 * (1.0 - alpha));
  smoothCh4 = (raw4 * alpha) + (smoothCh4 * (1.0 - alpha));
  smoothCh7 = (raw7 * alpha) + (smoothCh7 * (1.0 - alpha));

  // 3. Deadzone for Centers (Prevents "Servo Creep")
  #ifdef USE_DEADZONE
    if (abs((int)smoothCh1 - NEUTRAL) < deadzoneRange) smoothCh1 = NEUTRAL;
    if (abs((int)smoothCh2 - NEUTRAL) < deadzoneRange) smoothCh2 = NEUTRAL;
  #endif

  // 4. Mixing Logic / Channel Mapping
  // Ch1=Aileron, Ch2=Elevator/Throttle, Ch3=Throttle/Esc
  if (digitalRead(MIXER_SWITCH_PIN) == LOW) {
    // Tank Mix (Differential Drive)
    int throttle = (int)smoothCh2;
    int steering = (int)smoothCh1;
    int motorL = throttle + (steering - NEUTRAL);
    int motorR = throttle - (steering - NEUTRAL);

    payload.channels[0] = (byte)steering;           // Ch1
    payload.channels[1] = (byte)constrain(motorL, 0, 255); // Ch2
    payload.channels[2] = (byte)constrain(motorR, 0, 255); // Ch3
  } else {
    // Normal Pass-through
    payload.channels[0] = (byte)smoothCh1;
    payload.channels[1] = (byte)smoothCh2;
    payload.channels[2] = (byte)smoothCh3;
  }

  // Auxiliary Channels
  payload.channels[3] = (byte)smoothCh4;             // Ch4
  payload.channels[4] = !digitalRead(BUTTON_CH5_PIN) ? 255 : 0; // Ch5 (Digital)
  payload.channels[5] = !digitalRead(BUTTON_CH6_PIN) ? 255 : 0; // Ch6 (Digital)
  payload.channels[6] = (byte)smoothCh7;             // Ch7 (Pot)
}

void setup() {
  Serial.begin(9600);
  radio.begin();
  radio.setDataRate(RF24_250KBPS);
  radio.enableAckPayload();    // Enable Telemetry
  radio.setRetries(0, 15);     // High-speed RC retries
  radio.openWritingPipe(radioPipe);
  
  pinMode(MIXER_SWITCH_PIN, INPUT_PULLUP);
  pinMode(BUTTON_CH5_PIN, INPUT_PULLUP);
  pinMode(BUTTON_CH6_PIN, INPUT_PULLUP);

  if (digitalRead(BUTTON_CH5_PIN) == LOW) run_calibration();
  else { EEPROM.get(EE_ADDR_X_MIN, xMin); EEPROM.get(EE_ADDR_X_MAX, xMax);
         EEPROM.get(EE_ADDR_Y_MIN, yMin); EEPROM.get(EE_ADDR_Y_MAX, yMax); }
  
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
