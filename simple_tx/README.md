# 🕹️ AVR-RC Transmitter (Nano/Uno)

A high-performance, 7-channel Arduino-based RC transmitter utilizing the **nRF24L01** module for low-latency communication. This version is optimized for the **Arduino Nano** and features automated calibration and unique ID pairing (Bind Mode).

## 🔌 Hardware Pinout

### 1. NRF24L01 Radio Module

| NRF24 Pin | Nano Pin | Notes |
| :--- | :--- | :--- |
| **GND** | **GND** | Common ground |
| **VCC** | **3.3V** | **CRITICAL:** 3.3V only (Use a 10uF cap across VCC/GND) |
| **CE** | **D9** |  |
| **CSN** | **D10** |  |
| **SCK** | **D13** | Hardware SPI Clock |
| **MOSI** | **D11** | Hardware SPI Data In |
| **MISO** | **D12** | Hardware SPI Data Out |

### 2. Control Inputs

| Input Type | Nano Pin | Channel Mapping |
| :--- | :--- | :--- |
| **Joystick Left (X)** | **A0** | CH1 (Aileron / Steering) |
| **Joystick Left (Y)** | **A1** | CH2 (Elevator / Throttle L) |
| **Joystick Right (X)** | **A2** | CH3 (Throttle / Throttle R) |
| **Joystick Right (Y)** | **A3** | CH4 (Rudder) |
| **Button A** | **D2** | CH5 (Calibration Trigger) |
| **Button B** | **D3** | CH6 (Bind Trigger) |
| **Potentiometer** | **A4** | CH7 (Auxiliary) |
| **Mixer Switch** | **D4** | GND = Tank Mode / OPEN = Normal |

---

## 🚀 Operations

### 1. How to Bind (Pairing)
1. Set your **Receiver** to Bind Mode (A1 to GND at boot).
2. Hold **Button B (D3)** on the Transmitter and power it on.
3. The LED (D13) will flash rapidly for 5 seconds as it broadcasts its Unique ID.
4. Power cycle both units. They are now permanently paired.

### 2. How to Calibrate Sticks
1. Hold **Button A (D2)** on the Transmitter and power it on.
2. When the LED flashes, "stir" **both joysticks** in full circles, hitting all corners for 5 seconds.
3. Release sticks to their **natural center** and wait for the LED to stop.
4. All Min, Max, and Center-Deadzone values are now saved to EEPROM.

### 3. Telemetry
The transmitter automatically listens for "Ack Payloads" from the receiver.
*   **Data Sent:** Raw 10-bit Voltage.
*   **Processing:** The Nano calculates the voltage using the `3.127` multiplier (10k/4.7k divider).
*   **Display:** View real-time battery health via Serial Monitor (9600 baud) in `DEBUG_MODE`.

---

## 🛠️ Technical Specs
*   **Frequency:** 2.4GHz (NRF24L01)
*   **Data Rate:** 250KBPS (Optimized for range)
*   **Protocol:** 7-Channel Byte Array (Packed Struct)
*   **Smoothing:** Alpha Filter (0.2) for jitter-free flight.
