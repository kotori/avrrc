# 🕹️ AVR-RC Simple Transmitter (Nano/Uno)

This directory contains the **Simple TX** implementation for the AVR-RC project. It is optimized for the **Arduino Nano** and provides a high-performance 7-channel link with automated calibration and unique ID pairing.

**Location:** `https://github.com/kotori/avrrc/tree/main/simple_tx`

## 🔌 Hardware Pinout


| Component | Arduino Nano Pin | Notes |
| :--- | :--- | :--- |
| **NRF24 CE** | **D9** | |
| **NRF24 CSN** | **D10** | |
| **NRF24 SCK** | **D13** | Shared with Status LED |
| **NRF24 MOSI** | **D11** | |
| **NRF24 MISO** | **D12** | |
| **Joystick L-X** | **A0** | CH1 (Aileron / Steering) |
| **Joystick L-Y** | **A1** | CH2 (Elevator / Tank-Left) |
| **Joystick R-X** | **A2** | CH3 (Throttle / Tank-Right) |
| **Joystick R-Y** | **A3** | CH4 (Rudder) |
| **Button A** | **D2** | **Calibration Trigger** (Hold at Boot) |
| **Button B** | **D3** | **Bind Trigger** (Hold at Boot) |
| **Mixer Switch** | **D4** | GND = Tank Mode / Open = Normal |
| **Potentiometer**| **A4** | CH7 (Auxiliary) |

---

## 🚀 Essential Operations

### 1. Bind Mode (Unique ID Handshake)
To prevent interference from other transmitters:
1. Put your **Receiver** into Bind Mode.
2. Hold **Button B (D3)** on this Transmitter and power on.
3. The LED will flash rapidly for 5s as it broadcasts its Unique ID.
4. The Receiver will learn and save this ID to its EEPROM.

### 2. Full 8-Point Calibration
To ensure precise control and eliminate stick drift:
1. Hold **Button A (D2)** on this Transmitter and power on.
2. When the LED flashes, rotate **both joysticks** in full circles for 5 seconds.
3. Release sticks to their **natural center** and wait 1 second.
4. The system saves all Min, Max, and Center-Deadzone values to EEPROM.

### 3. Integrated Mixing
The code features a built-in differential mixer:
*   **Normal Mode (Switch Open):** Direct pass-through for planes/drones.
*   **Tank Mode (Switch to GND):** Automatically mixes Throttle (CH2) and Steering (CH1) for differential-drive robots.

---

## 🛠️ Technical Details
*   **Smoothing:** Alpha Filter (0.2) reduces jitter from analog sticks.
*   **Deadzone:** 4-unit software deadzone centered on your unique joystick rest position.
*   **Telemetry:** Automatically receives and displays raw RX battery voltage via Serial Monitor (9600 baud).
