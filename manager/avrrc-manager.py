#!/usr/bin/env python3
import serial
import struct
import json
import sys
import os

# CONFIGURATION
SERIAL_PORT = "/dev/ttyUSB0"  # Adjust if your Mega is on /dev/ttyACM0
BAUD_RATE = 9600
FLEET_SIZE = 20
# Struct: 12s (Name), 4i (Calibration), 4i (Trims)
STRUCT_FORMAT = "<12s i i i i 4i"
STRUCT_SIZE = struct.calcsize(STRUCT_FORMAT)

def get_fleet(filename):
    print(f"Connecting to {SERIAL_PORT}...")
    try:
        with serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=5) as ser:
            ser.write(b'G')  # Command 'G'et Fleet
            fleet = []
            for i in range(FLEET_SIZE):
                raw = ser.read(STRUCT_SIZE)
                if len(raw) < STRUCT_SIZE:
                    print(f"Error: Timeout reading model {i}")
                    break
                
                data = struct.unpack(STRUCT_FORMAT, raw)
                name = data[0].decode('ascii', errors='ignore').strip('\x00')
                fleet.append({
                    "slot": i,
                    "name": name,
                    "cal": list(data[1:5]),
                    "trims": list(data[5:])
                })
            
            with open(filename, 'w') as f:
                json.dump(fleet, f, indent=4)
            print(f"Successfully backed up {len(fleet)} models to {filename}")
    except Exception as e:
        print(f"Connection failed: {e}")

def set_fleet(filename):
    if not os.path.exists(filename):
        print(f"Error: {filename} not found.")
        return

    with open(filename, 'r') as f:
        fleet = json.load(f)

    print(f"Uploading {filename} to Transmitter...")
    try:
        with serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=10) as ser:
            ser.write(b'S')  # Command 'S'et Fleet
            for m in fleet:
                # Prepare binary packet
                raw = struct.pack(STRUCT_FORMAT, 
                                  m['name'].encode('ascii')[:11], 
                                  *m['cal'], 
                                  *m['trims'])
                ser.write(raw)
            print("Upload complete! Transmitter is rebooting fleet memory.")
    except Exception as e:
        print(f"Sync failed: {e}")

def usage():
    print("AVRRC Fleet Manager")
    print("Usage: ./avrrc-manager.py [get|set] [filename.json]")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        usage()
    elif sys.argv[1] == "get":
        get_fleet(sys.argv[2])
    elif sys.argv[1] == "set":
        set_fleet(sys.argv[2])
    else:
        usage()
