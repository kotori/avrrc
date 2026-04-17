/*
  AVRRC
  Receiver code for the Arduino Radio control with PWM or PPM output via a nRF24
  module.

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
      1   ->    D2
      2   ->    D3
      3   ->    D4
      4   ->    D5
      5   ->    D6
      6   ->    D7
      7   ->    D8
*/

#include <EEPROM.h>
#include <RF24.h>
#include <SPI.h>
#include <Servo.h>
#include <nRF24L01.h>

// --- PINS ---
const int STATUS_LED_PIN = A2;
const int BIND_FAILSAFE_PIN = A1;  // Dual purpose: GND at boot = BIND
const int VOLTAGE_PIN = A0;
const byte outputPins[] = {2, 3, 4, 5, 6, 7, 8};

// --- GLOBALS ---
RF24 radio(9, 10);
Servo servoOut[7];
uint64_t masterAddress;  // The unique ID learned from TX

struct __attribute__((packed)) Payload { 
  byte ch1, ch2, ch3, ch4, ch5, ch6, ch7; 
};

struct __attribute__((packed)) Telemetry { 
  float voltage; 
  bool signalOk; 
};

unsigned long lastRecvTime = 0, lastBlinkTime = 0;
bool ledState = false;
const byte NEUTRAL = 127;

// --- BINDING LOGIC: LISTEN AND LEARN ---
void handle_binding() {
  radio.begin();
  radio.setDataRate(RF24_250KBPS);
  radio.openReadingPipe(1, 0x0000000001LL);  // Listen on global Bind Channel
  radio.startListening();

  while (true) {
    // Rapid blink while waiting for TX
    if (millis() - lastBlinkTime > 100) {
      ledState = !ledState;
      digitalWrite(STATUS_LED_PIN, ledState);
      lastBlinkTime = millis();
    }

    if (radio.available()) {
      radio.read(&masterAddress, sizeof(masterAddress));
      EEPROM.put(0, masterAddress);  // Save new ID to address 0

      // Success: Solid LED for 3 seconds
      digitalWrite(STATUS_LED_PIN, HIGH);
      delay(3000);
      break;
    }
  }
}

void setup() {
  pinMode(STATUS_LED_PIN, OUTPUT);
  pinMode(BIND_FAILSAFE_PIN, INPUT_PULLUP);

  // 1. Check for Bind Jumper (A1 to GND at boot)
  if (digitalRead(BIND_FAILSAFE_PIN) == LOW) {
    handle_binding();
  }

  // 2. Load Master ID from EEPROM
  EEPROM.get(0, masterAddress);

  // Safety: If EEPROM is empty, use a default so it's not a dead brick
  if (masterAddress == 0 || masterAddress == 0xFFFFFFFFFFFFFFFFLL) {
    masterAddress = 0xE8E8F0F0E1LL;
  }

  // 3. Normal Startup
  radio.begin();
  radio.setDataRate(RF24_250KBPS);
  radio.enableAckPayload();
  radio.openReadingPipe(1, masterAddress);

  telemetry.voltage = 0.0;
  telemetry.signalOk = true;
  radio.writeAckPayload(1, &telemetry, sizeof(telemetry));

  radio.startListening();

  for (int i = 0; i < 7; i++) {
    servoOut[i].attach(outputPins[i]);
  }

  resetPayload();
  updateHardware();
}

void updateTelemetry() {
  int raw = analogRead(VOLTAGE_PIN);
  telemetry.voltage = raw * (5.0 / 1024.0) * 3.127;
  telemetry.signalOk = (millis() - lastRecvTime < 500);

  radio.writeAckPayload(1, &telemetry, sizeof(telemetry));
}

void loop() {
  unsigned long now = millis();

  if (radio.available()) {
    radio.read(&payload, sizeof(Payload));
    lastRecvTime = now;

    digitalWrite(STATUS_LED_PIN, HIGH);
    updateTelemetry(); 
    updateHardware(); // Moves servos to new positions
  }

  // --- SIGNAL BLACKOUT MEMORY ---
  // If we haven't heard anything in 500ms, trigger full failsafe
  if (now - lastRecvTime > 500) {
    resetPayload();
    updateHardware();
    
    telemetry.signalOk = false;
    radio.writeAckPayload(1, &telemetry, sizeof(telemetry));

    // Slow blink for Failsafe
    if (now - lastBlinkTime > 200) {
      ledState = !ledState;
      digitalWrite(STATUS_LED_PIN, ledState);
      lastBlinkTime = now;
    }
  } 
  // If we miss a few packets (e.g. between 100ms and 500ms)
  else if (now - lastRecvTime > 100) {
    // HOLD last known good position (Don't call updateHardware or resetPayload)
    // Rapid blink to show "Dirty Link"
    if (now - lastBlinkTime > 50) {
      ledState = !ledState;
      digitalWrite(STATUS_LED_PIN, ledState);
      lastBlinkTime = now;
    }
  }
}

void updateHardware() {
  for (int i = 0; i < 7; i++) {
    byte val = *(((byte*)&payload) + i);
    int pulse = map(val, 0, 255, 1000, 2000);
    servoOut[i].writeMicroseconds(pulse);
  }
}

void resetPayload() {
  payload.ch1 = payload.ch4 = payload.ch7 = NEUTRAL;
  bool fullStop = (digitalRead(BIND_FAILSAFE_PIN) == LOW);
  payload.ch2 = fullStop ? 0 : NEUTRAL;
  payload.ch3 = fullStop ? 0 : NEUTRAL;
  payload.ch5 = 0;
  payload.ch6 = 0;
}
