# ⚓ AVRRC: Open-Source RC Control System
**High-resolution, low-latency control for boats and modular models.**

AVRRC is a custom-engineered 2.4GHz radio system built for the Arduino ecosystem. It focuses on precision control, robust telemetry, and modular hardware configurations.

---

## 🚀 Quick Start (v1.0 Stealth Brick)

Follow these steps to get your "Stealth Brick" system from the bench to the water.

### 1. Firmware Deployment
* **Transmitter:** Flash `tx-mega.ino` to your Arduino Mega 2560 Pro Mini.
* **Receiver:** Flash `rx.ino` to your Arduino Nano.
* **Bind:** Hold **Button A + B** on the TX while powering on the RX with the **A1 Jumper** connected to GND.

### 2. Build the Desktop Manager (Debian/Linux)
Ensure you have the Qt6 development headers installed:
```bash
sudo apt install qt6-base-dev qt6-serialport-dev qt6-sql-dev cmake build-essential
cd manager/manager-cpp
mkdir build && cd build
cmake ..
make
./AVRRC_Manager
```

Connect via USB and click Sync from TX to pull your 20 model slots into the local SQLite library.

### 3. Operation & Safety Handshake
he system boots in LOCKED mode for safety. You cannot spin the motors until you "Arm" the transmitter:

1. Power on the Transmitter and Boat.
2. Pull the Left Stick (Throttle) all the way DOWN.
3. Press Button A.
4. The buzzer will beep, and the OLED will switch from ! LOCKED ! to the live dashboard. You are now live.

#### ⚓ Core Controls

 * Stick Left (A1/A0): Throttle & Steering (Tank/Mixer mode available via D4 switch).
 * Stick Right (A3/A2): Auxiliary Channels (Gimbals, Cranes, etc.).
 * Trims: Use the Side Buttons (D6, D7, D8) to nudge digital trims in real-time. Settings auto-save to EEPROM after 5 seconds of inactivity.

---

## 🎮 Transmitter (TX) Configurations

### Option A: Standard TX (Arduino Nano)
*Optimized for lightweight, compact handheld operation.*


| Component | Pin | Type | Logic / Function |
| :--- | :--- | :--- | :--- |
| **Joystick 1 (X)** | `A0` | Analog | Steering / Rudder |
| **Joystick 1 (Y)** | `A1` | Analog | Throttle / Left Motor |
| **Button A** | `D2` | Digital | Ch 5 (Hold at boot to Calibrate) |
| **Mixer Switch** | `D4` | Digital | **GND** = Tank Mixing ON |
| **Radio SPI** | `D11-D13`| SPI | MOSI (11), MISO (12), SCK (13) |

### Option B: Advanced TX (Arduino Mega 2560)
*The "Stealth Brick" configuration: OLED Dashboard and Multi-Model Memory.*


| Component | Pin | Function |
| :--- | :--- | :--- |
| **Gimbal 1 (X/Y)** | `A0` / `A1` | Primary Steering & Throttle |
| **Gimbal 2 (X/Y)** | `A2` / `A3` | Auxiliary / Pan-Tilt / Yaw |
| **Trim Select** | `D8` | Cycles through CH1 - CH4 for tuning |
| **Trim (+ / -)** | `D6` / `D7` | Adjusts software center of selected channel |
| **Button A** | `D2` | Ch 5 (Hold at boot for Calibration) |
| **Button B** | `D3` | Ch 6 (Hold at boot for Model Selection) |
| **Mixer Switch** | `D4` | **GND** = Dual-Motor Tank Mixing ON |
| **Buzzer (+)** | `D5` | Audible feedback and nautical alarms |
| **TX Batt Sense**| `A15` | Voltage monitor (10k/10k divider) |
| **OLED I2C** | `20` / `21` | SDA (20) / SCL (21) |
| **Radio SPI** | `50-52` | MISO (50), MOSI (51), SCK (52) |

---

## ⚓ Receiver Connection Map (Arduino Nano / Uno)

The receiver manages the 2.4GHz link, monitors the main battery, and drives up to 7 servos or ESCs.

### 🔌 Servo & ESC Outputs (Channels 1-7)


| Arduino Pin | Payload Channel | Typical Application |
| :--- | :--- | :--- |
| **D2** | `ch1` | Rudder Servo (Steering) |
| **D3** | `ch2` | Main Motor ESC (Throttle) |
| **D4** | `ch3` | Secondary Motor ESC (Mixer) |
| **D5** | `ch4` | Auxiliary Servo 1 (Yaw/Pan) |
| **D6** | `ch5` | Auxiliary Servo 2 (Tilt) |
| **D7** | `ch6` | Digital Switched Output 1 |
| **D8** | `ch7` | Digital Switched Output 2 |

### 🛠️ Sensors & Indicators


| Arduino Pin | Function | Wiring Note |
| :--- | :--- | :--- |
| **A0** | **Voltage Sense** | Center tap of 10k/4.7k divider (Max 15V) |
| **A1** | **Failsafe / Bind** | Connect to GND to Bind at boot |
| **A2** | **Status LED** | Connect to LED (+) via 220Ω resistor |

### 📡 nRF24L01+ Radio (Hardware SPI)


| nRF24 Pin | Arduino Nano Pin | Wiring Note |
| :--- | :--- | :--- |
| **VCC** | **3.3V** | **CRITICAL: DO NOT USE 5V** |
| **GND** | **GND** | Common ground with Nano |
| **CE** | **D9** | |
| **CSN** | **D10** | |
| **SCK** | **D13** | Hardware SPI Clock |
| **MOSI** | **D11** | Hardware SPI Data In |
| **MISO** | **D12** | Hardware SPI Data Out |

> [!CAUTION]
> **Timer Conflict:** On the Arduino Nano/Uno, using the `Servo` library disables `analogWrite()` (PWM) on **Pins 9 and 10**. This is perfectly fine for your build as these pins are used for the radio's `CE` and `CSN` signals.

---

## 📡 Radio Module (nRF24L01) Pinout

> [!WARNING]
> **VCC MUST be 3.3V.** Connecting the nRF24L01 to 5V will permanently damage the module.

### Common Power Pins


| nRF Pin | Arduino Pin | Note |
| :--- | :--- | :--- |
| **GND** | **GND** | Common Ground |
| **VCC** | **3.3V** | **MAX 3.6V** |
| **CE** | **D9** | Chip Enable |
| **CSN** | **D10** | Chip Select |

### Platform Data Pins


| Function | Arduino Nano (RX/TX) | Arduino Mega (Advanced TX) |
| :--- | :--- | :--- |
| **SCK** (Clock) | `D13` | `D52` |
| **MOSI** | `D11` | `D51` |
| **MISO** | `D12` | `D50` |

---

## ⚓ Standardized Channel Map (Mode 2)

To ensure consistent operation, AVRRC follows the industry-standard **Mode 2** gimbal layout. This ensures that movement logic remains on the Left Stick while the Right Stick is reserved for secondary controls.

### 🕹️ Transmitter Input Layout
*   **Left Stick (X-Axis):** Yaw (Rudder / Steering)
*   **Left Stick (Y-Axis):** Throttle (Forward/Reverse)
*   **Right Stick (X-Axis):** Auxiliary 1 (Pan / Camera / Accessories)
*   **Right Stick (Y-Axis):** Auxiliary 2 (Tilt / Lights / Crane)

### 🔌 Receiver Output Mapping

| Channel | Standard Mode (Mixer OFF) | Tank Mode (Mixer ON) |
| :--- | :--- | :--- |
| **CH1** | Rudder Servo | Rudder Servo (Inactive) |
| **CH2** | Main Motor ESC | **Left Motor ESC** |
| **CH3** | Auxiliary Servo 1 | **Right Motor ESC** |
| **CH4** | Auxiliary Servo 2 | Auxiliary Servo 2 |
| **CH5** | Button A (Digital) | Button A (Digital) |
| **CH6** | Button B (Digital) | Button B (Digital) |
| **CH7** | Mixer OFF (0) | Mixer ON (255) |

> [!TIP]
> **Tank Mode Logic:** When the Mixer Switch (D4) is grounded, the Left Stick's Throttle and Steering are combined to drive CH2 and CH3. This allows you to drive a dual-motor boat entirely with your left thumb.

---

## 🚀 Operational Guide

### ⚖️ Auto-Calibration
To map your joysticks to the full 0–255 software resolution:
1. **Hold Button A (D2)** while powering on the transmitter.
2. Move both sticks in their full range of motion for 5 seconds.
3. Limits are automatically saved to EEPROM and persist after power-down.

### ⚙️ Stealth Trims
Adjust your model's center-point without looking at the screen:
1. Press **Trim Select (D8)** to cycle through Channels 1-4 (indicated by buzzer tones).
2. Use **Trim +/- (D6/D7)** to nudge the center.
3. Values save automatically after 5 seconds of inactivity.

### 🛥️ Model Memory (Mega Only)
Manage up to **20 independent boat profiles**:
*   **Switching:** Hold **Button B (D3)** at power-on. Tap Button B within 5 seconds to cycle profiles.
*   **Renaming:** Connect via USB, open Serial Monitor (9600 baud), and type a name (Max 11 chars).

### 📶 Telemetry & Links
The Advanced TX dashboard provides live diagnostics:
*   **RX BATT:** Real-time boat battery voltage via ACK-Payload.
*   **TX BATT:** Transmitter battery health (A15 sense).
*   **LINK:** Signal quality (%) based on packet success rate.

---

## 🛡️ Hardware Safety & Requirements

1.  **Radio Power:** The nRF24L01 **MUST** use **3.3V**.
2.  **Decoupling Capacitor:** Solder a **10µF to 100µF capacitor** directly across the radio's `VCC` and `GND` pins.
3.  **Common Ground:** The boat battery, Arduino, and ESCs **MUST** share a common **GND**.

### 📦 Required Libraries
*   **RF24** (by TMRh20)
*   **U8g2** (by oliver) - *Mega TX only*
*   **Servo** (Built-in)

---

## ⚡ Battery Telemetry (Voltage Dividers)

Both the Transmitter and Receiver monitor battery health using Voltage Dividers. These scale high battery voltages down to the 0–5V range for the Arduino analog pins.

### 🛥️ Receiver (RX) Telemetry
Designed for up to **3S LiPo (12.6V)**.
*   **Resistors:** R1 = 10kΩ | R2 = 4.7kΩ
*   **Multiplier:** `3.127`
*   **Formula:** `(10000 + 4700) / 4700 = 3.127`

### 🎮 Transmitter (TX) Telemetry
Designed for **2S LiPo/Li-ion (8.4V)**.
*   **Resistors:** R1 = 10kΩ | R2 = 10kΩ
*   **Multiplier:** `2.0`
*   **Formula:** `(10000 + 10000) / 10000 = 2.0`

### 🛠️ Wiring Diagram (Universal)
```text
       [ Battery (+) ] 


              |
          [ 10kΩ ] (R1)
              |
Pin <---------+--- (Tap to A0/A15)

              |
          [ R2 ] (4.7k or 10k)
              |
       [ Common GND ]
```
>[!WARNING]
>Common Ground: You MUST connect the Battery Negative (-) to the Arduino GND. 
>Without this common reference, the telemetry readings will be erratic or zero.

## 🏷️ Model Renaming (USB)
Renaming applies to the **currently active** model index. All names are limited to **11 characters**.

### ⌨️ Terminal Commands
*   **Arduino Serial Monitor:** 9600 Baud | Line ending set to **Newline**.
*   **Linux CLI:** `stty -F /dev/ttyUSB0 9600 raw && echo "BOATNAME" > /dev/ttyUSB0`
*   **PuTTY:** Serial | 9600 | Category > Terminal > Check "Implicit LF on every CR".
*   **Screen:** `screen /dev/ttyUSB0 9600` (Use `Ctrl+J` to send the required Line Feed).

---

## 🔋 Power Systems & Recommendations


| Setup | Recommended Battery | Why? |
| :--- | :--- | :--- |
| **Transmitter** | **2S (7.4V) Li-ion or LiPo** | Stable voltage for hours of operation; easy to regulate. |
| **Receiver (2S)** | **7.4V LiPo** | Ideal for standard scale boats and servos. |
| **Receiver (3S)** | **11.1V LiPo** | Necessary for high-speed brushless motor "punch." |

*   **Wiring:** Connect battery to **VIN** on Arduino (7-12V safe range).
*   **Servos:** Use a dedicated **BEC** (Battery Eliminator Circuit) for high-load servos to prevent MCU brown-outs.

---

## 🪵 The "Stealth Brick" Enclosure

Custom 10-layer plywood enclosure designed for nautical ergonomics and industrial durability.

*   **Material:** 10 layers of **1/4" (6.35mm) Plywood**.
*   **Construction:** Layers 3-9 are hollowed for internal electronics.
*   **Nautical Finish:** Through-bolted 5" boat cleat acts as a handle and antenna roll-cage.

### 📐 Faceplate Template (SVG)

<p align="center">
  <img src="images/case-top.svg" width="400" alt="Faceplate Template">
</p>


## 🎮 Transmitter Schematic (Advanced TX)

### 📐 Transmitter Schematic (SVG)

<p align="center">
  <img src="images/tx-mega-schematic.svg" width="400" alt="TX Mega Template">
</p>


## 🎮 Receiver Schematic (Nano)

This shows how the Arduino Nano distributes signals to your 7 Servo Channels.

### 📐 Receiver Schematic (SVG)

<p align="center">
  <img src="images/rx-schematic.svg" width="400" alt="RX Template">
</p>

