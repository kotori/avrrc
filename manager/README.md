---

## 🖥️ Linux Companion Software (Fleet Manager)
While the Transmitter (TX) handles **20 active models** in its internal memory, the Linux Companion App allows for an **infinite global library** of boat profiles. This tool enables you to backup, reorder, and manually edit model settings (like trims) via a JSON-based master list.

### 🚀 Features
- **Infinite Storage:** Save different "Fleet" files (e.g., `racing_fleet.json`, `tugboat_fleet.json`).
- **Manual Editing:** Modify boat names or fine-tune trim values via a text editor.
- **Fleet Reordering:** Swap model positions by simply moving JSON blocks.
- **One-Click Sync:** Flash a full 20-model configuration to the TX in seconds.

### 📦 Prerequisites (Debian/Ubuntu)
The manager requires `python3` and the `pyserial` library:
```bash
sudo apt update
sudo apt install python3-serial
```

The GUI manager requires `python3`, `pyserial`, and `customtkinter` 

### ⌨️ CLI Usage
Run the manager from your terminal. Ensure your Stealth Brick is connected via USB.
|Command|Action|
|-------|------|
|./avrrc-manager.py get my_fleet.json|Download all 20 models from TX to your PC.|
|./avrrc-manager.py set my_fleet.json|Upload a JSON fleet file to the Transmitter.|

### 📄 The JSON Database Format
The fleet is stored in a human-readable format. You can manually edit these values to "nudge" trims or rename boats without using the transmitter menus.
```json
{
    "slot": 0,
    "name": "STEALTH_1",
    "cal": [0, 1023, 0, 1023],
    "trims": [2, -5, 0, 0]
}
```

### 🛠️ Communication Protocol (Binary Sync)
The Transmitter and PC communicate using a specific binary handshake to ensure data integrity during fleet transfers.

 * Command 'G' (Get): TX dumps the entire 20-model EEPROM as a raw binary stream to the PC.
 * Command 'S' (Set): TX enters "Sync Mode," receives 20 binary packets from the PC, and overwrites the internal EEPROM.
 * Buzzer Feedback: The TX will emit a Low Beep at the start of a sync and a High Beep once the flash is successful.

>[!WARNING]
>Sync Safety: Avoid moving the transmitter sticks during a Set operation. The PC-Sync temporarily disables radio interrupts to prevent data corruption during the EEPROM flash.


