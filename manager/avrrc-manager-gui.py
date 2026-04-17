import customtkinter as ctk
import serial, struct, json, threading

# Configuration
SERIAL_PORT = "/dev/ttyUSB0"
BAUD_RATE = 9600
# Updated Struct: 12s (Name), Q (Address), 4i (Cal), 4i (Trims)
STRUCT_FORMAT = "<12s Q i i i i 4i" 
STRUCT_SIZE = struct.calcsize(STRUCT_FORMAT)

class ModelEditor(ctk.CTkToplevel):
    def __init__(self, master, model_data, on_save):
        super().__init__(master)
        self.title(f"Editing Slot {model_data['slot']}")
        self.geometry("400x650")
        self.on_save = on_save
        self.model_data = model_data

        # Name Entry
        ctk.CTkLabel(self, text="Model Name:").pack(pady=(15, 0))
        self.name_entry = ctk.CTkEntry(self, width=200)
        self.name_entry.insert(0, model_data['name'])
        self.name_entry.pack(pady=5)

        # Address Entry (Hex format)
        ctk.CTkLabel(self, text="Bind Address (Hex):").pack(pady=(15, 0))
        self.addr_entry = ctk.CTkEntry(self, width=200)
        self.addr_entry.insert(0, hex(model_data['addr']))
        self.addr_entry.pack(pady=5)

        # Trim Sliders
        self.trim_sliders = []
        for i in range(4):
            ctk.CTkLabel(self, text=f"Ch {i+1} Trim").pack(pady=(10, 0))
            slider = ctk.CTkSlider(self, from_=-40, to=40, number_of_steps=80)
            slider.set(model_data['trims'][i])
            slider.pack(pady=5)
            self.trim_sliders.append(slider)

        self.save_btn = ctk.CTkButton(self, text="Apply Changes", fg_color="green", command=self.save)
        self.save_btn.pack(pady=20)
        self.grab_set()

    def save(self):
        try:
            addr_val = int(self.addr_entry.get(), 16) # Convert hex string to int
            self.on_save(
                self.model_data['slot'], 
                self.name_entry.get()[:11], 
                addr_val,
                [int(s.get()) for s in self.trim_sliders]
            )
            self.destroy()
        except ValueError:
            self.addr_entry.configure(border_color="red")

class FleetManager(ctk.CTk):
    def __init__(self):
        super().__init__()
        self.title("AVRRC Stealth Brick - Pro Manager")
        self.geometry("600x650")
        ctk.set_appearance_mode("dark")
        self.fleet = []

        self.scroll_frame = ctk.CTkScrollableFrame(self, width=500, height=400, label_text="Fleet List")
        self.scroll_frame.pack(pady=20)

        self.btn_get = ctk.CTkButton(self, text="Sync from TX", command=lambda: self.start_thread(self.sync_from_tx))
        self.btn_get.pack(pady=5)

        self.btn_set = ctk.CTkButton(self, text="Upload to TX", fg_color="orange", command=lambda: self.start_thread(self.upload_to_tx))
        self.btn_set.pack(pady=5)

        self.status = ctk.CTkLabel(self, text="Status: Ready", text_color="gray")
        self.status.pack(side="bottom", pady=10)

    def start_thread(self, target_func):
        threading.Thread(target=target_func, daemon=True).start()

    def sync_from_tx(self):
        try:
            with serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=5) as ser:
                ser.write(b'G')
                new_fleet = []
                for i in range(20):
                    raw = ser.read(STRUCT_SIZE)
                    data = struct.unpack(STRUCT_FORMAT, raw)
                    new_fleet.append({
                        "slot": i, 
                        "name": data[0].decode('ascii', errors='ignore').strip('\x00'),
                        "addr": data[1],
                        "cal": list(data[2:6]), 
                        "trims": list(data[6:])
                    })
                self.fleet = new_fleet
                self.after(0, self.refresh_list)
        except Exception as e:
            self.after(0, lambda: self.status.configure(text=f"Error: {str(e)}", text_color="red"))

    def upload_to_tx(self):
        try:
            with serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=10) as ser:
                ser.write(b'S')
                for m in self.fleet:
                    raw = struct.pack(STRUCT_FORMAT, m['name'].encode('ascii')[:11], m['addr'], *m['cal'], *m['trims'])
                    ser.write(raw)
                self.after(0, lambda: self.status.configure(text="Sync Successful", text_color="green"))
        except Exception as e:
            self.after(0, lambda: self.status.configure(text=f"Error: {str(e)}", text_color="red"))

    def refresh_list(self):
        for w in self.scroll_frame.winfo_children(): w.destroy()
        for m in self.fleet:
            btn = ctk.CTkButton(self.scroll_frame, text=f"[{m['slot']}] {m['name']} ({hex(m['addr'])})", 
                                anchor="w", command=lambda x=m: ModelEditor(self, x, self.update_model_data))
            btn.pack(fill="x", pady=2)

    def update_model_data(self, slot, name, addr, trims):
        self.fleet[slot].update({"name": name, "addr": addr, "trims": trims})
        self.refresh_list()

if __name__ == "__main__":
    app = FleetManager()
    app.mainloop()
