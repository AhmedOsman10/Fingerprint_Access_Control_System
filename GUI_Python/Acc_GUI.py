"""
STM32 Fingerprint Access Control GUI
------------------------------------
Requirements:
    pip install pyserial

This GUI communicates with the STM32 using the frame format:

[0] SOF  = 0xAA
[1] CMD  = Command ID
[2] LEN  = payload length
[3..] DATA
[last] CS = CMD ^ LEN ^ DATA[0] ^ ... ^ DATA[N-1]

Supported commands:
    0x10 : PC -> STM32  : Enrollment request
    0x11 : STM32 -> PC  : Enrollment / GUI instruction
    0x12 : STM32 -> PC  : Access log

Design notes:
- Non-blocking serial polling using Tkinter after()
- Robust frame parser with internal RX buffer
- Safe disconnect handling if cable is unplugged
- Start Enrollment button disabled until connected
"""

import struct
import tkinter as tk
from tkinter import ttk, messagebox
from tkinter.scrolledtext import ScrolledText
import serial
import serial.tools.list_ports


class FingerprintAccessGUI:
    # =========================
    # Protocol constant
    # =========================
    SOF = 0xAA

    CMD_ENROLL_REQ = 0x10   # PC -> STM32
    CMD_ENROLL_ST = 0x11    # STM32 -> PC
    CMD_LOG_ACCESS = 0x12   # STM32 -> PC

    POLL_INTERVAL_MS = 50

    # =========================
    # GUI instruction mapping
    # Based on the STM32 file and your required UI behavior
    # =========================
    INSTRUCTION_MAP = {
        0: ("SEARCHING... (IDLE)", "#1f3a5f"),          # Dark Blue
        1: ("STEP 1: PLACE FINGER", "#f39c12"),         # Yellow/Orange
        2: ("STEP 2: LIFT FINGER", "#d35400"),          # Darker Orange
        3: ("STEP 3: PLACE AGAIN", "#f39c12"),          # Yellow/Orange
        4: ("PROCESSING... PLEASE WAIT", "#5dade2"),    # Light Blue
        5: ("ENROLLMENT SUCCESSFUL!", "#27ae60"),       # Green
        6: ("ENROLLMENT FAILED!", "#c0392b"),           # Red
        7: ("BAD SCAN! TRY AGAIN", "#e74c3c"),          # Red
    }

    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("Fingerprint Biometric Access System")
        self.root.geometry("900x620")
        self.root.minsize(760, 520)

        # Serial state
        self.ser = None
        self.is_connected = False

        # Raw receive buffer
        self.rx_buffer = bytearray()

        # Tk variables
        self.port_var = tk.StringVar()
        self.baud_var = tk.StringVar(value="115200")
        self.connection_status_var = tk.StringVar(value="Disconnected")

        # Build UI
        self._build_gui()

        # Initial UI state
        self.refresh_ports()
        self._set_connected_ui(False)
        self._update_banner(0)

        # Start polling loop
        self.root.after(self.POLL_INTERVAL_MS, self.poll_serial)

        # Window close handling
        self.root.protocol("WM_DELETE_WINDOW", self.on_close)

    # ==========================================================
    # GUI Construction
    # ==========================================================
    def _build_gui(self):
        self.root.configure(bg="#ecf0f1")

        # -------- Top banner --------
        self.banner_frame = tk.Frame(self.root, bg="#1f3a5f", height=90)
        self.banner_frame.pack(fill="x", padx=10, pady=(10, 8))
        self.banner_frame.pack_propagate(False)

        self.banner_label = tk.Label(
            self.banner_frame,
            text="SEARCHING... (IDLE)",
            font=("Segoe UI", 22, "bold"),
            bg="#1f3a5f",
            fg="white"
        )
        self.banner_label.pack(expand=True, fill="both")

        # -------- Connection panel --------
        conn_outer = tk.LabelFrame(
            self.root,
            text="Connection",
            font=("Segoe UI", 11, "bold"),
            bg="#ecf0f1",
            padx=10,
            pady=10
        )
        conn_outer.pack(fill="x", padx=10, pady=6)

        # Port
        tk.Label(conn_outer, text="COM Port:", font=("Segoe UI", 10), bg="#ecf0f1").grid(
            row=0, column=0, sticky="w", padx=(0, 6), pady=4
        )

        self.port_combo = ttk.Combobox(
            conn_outer,
            textvariable=self.port_var,
            state="readonly",
            width=30
        )
        self.port_combo.grid(row=0, column=1, sticky="w", padx=(0, 10), pady=4)

        self.refresh_btn = tk.Button(
            conn_outer,
            text="Refresh Ports",
            font=("Segoe UI", 10),
            command=self.refresh_ports,
            bg="#bdc3c7",
            relief="raised",
            padx=10
        )
        self.refresh_btn.grid(row=0, column=2, sticky="w", pady=4)

        # Baudrate
        tk.Label(conn_outer, text="Baudrate:", font=("Segoe UI", 10), bg="#ecf0f1").grid(
            row=1, column=0, sticky="w", padx=(0, 6), pady=4
        )

        self.baud_combo = ttk.Combobox(
            conn_outer,
            textvariable=self.baud_var,
            state="readonly",
            width=28,
            values=["9600", "19200", "38400", "57600", "115200", "230400"]
        )
        self.baud_combo.grid(row=1, column=1, sticky="w", padx=(0, 10), pady=4)

        self.connect_btn = tk.Button(
            conn_outer,
            text="Connect",
            font=("Segoe UI", 11, "bold"),
            command=self.toggle_connection,
            bg="#27ae60",
            fg="white",
            width=16,
            relief="raised"
        )
        self.connect_btn.grid(row=1, column=2, sticky="w", pady=4)

        # Connection status
        self.status_label = tk.Label(
            conn_outer,
            textvariable=self.connection_status_var,
            font=("Segoe UI", 10, "bold"),
            bg="#ecf0f1",
            fg="#c0392b"
        )
        self.status_label.grid(row=0, column=3, rowspan=2, padx=(25, 0), sticky="w")

        # -------- Action panel --------
        action_outer = tk.LabelFrame(
            self.root,
            text="Actions",
            font=("Segoe UI", 11, "bold"),
            bg="#ecf0f1",
            padx=10,
            pady=10
        )
        action_outer.pack(fill="x", padx=10, pady=6)

        self.enroll_btn = tk.Button(
            action_outer,
            text="Start Enrollment",
            font=("Segoe UI", 13, "bold"),
            command=self.send_enroll_request,
            bg="#2980b9",
            fg="white",
            activebackground="#3498db",
            width=22,
            height=2,
            relief="raised",
            state="disabled"
        )
        self.enroll_btn.pack(anchor="w")

        # -------- Log panel --------
        log_outer = tk.LabelFrame(
            self.root,
            text="System Log / Access Log",
            font=("Segoe UI", 11, "bold"),
            bg="#ecf0f1",
            padx=10,
            pady=10
        )
        log_outer.pack(fill="both", expand=True, padx=10, pady=(6, 10))

        self.log_text = ScrolledText(
            log_outer,
            wrap="word",
            font=("Consolas", 11),
            bg="#2c3e50",
            fg="#ecf0f1",
            insertbackground="white",
            relief="sunken",
            borderwidth=2
        )
        self.log_text.pack(fill="both", expand=True)
        self.log_text.config(state="disabled")

        self._log("Application started.")
        self._log("Select COM port and connect to STM32.")

    # ==========================================================
    # Logging helpers
    # ==========================================================
    def _log(self, message: str):
        self.log_text.config(state="normal")
        self.log_text.insert("end", message + "\n")
        self.log_text.see("end")
        self.log_text.config(state="disabled")

    def _update_banner(self, instruction_id: int):
        text, color = self.INSTRUCTION_MAP.get(
            instruction_id,
            (f"UNKNOWN INSTRUCTION: {instruction_id}", "#7f8c8d")
        )
        self.banner_frame.configure(bg=color)
        self.banner_label.configure(text=text, bg=color)

    def _set_connected_ui(self, connected: bool):
        self.is_connected = connected

        if connected:
            self.connection_status_var.set("Connected")
            self.status_label.config(fg="#27ae60")
            self.connect_btn.config(text="Disconnect", bg="#c0392b")
            self.enroll_btn.config(state="normal")
            self.port_combo.config(state="disabled")
            self.baud_combo.config(state="disabled")
        else:
            self.connection_status_var.set("Disconnected")
            self.status_label.config(fg="#c0392b")
            self.connect_btn.config(text="Connect", bg="#27ae60")
            self.enroll_btn.config(state="disabled")
            self.port_combo.config(state="readonly")
            self.baud_combo.config(state="readonly")
            self._update_banner(0)

    # ==========================================================
    # Serial connection
    # ==========================================================
    def refresh_ports(self):
        """Auto-detect available COM ports."""
        try:
            ports = serial.tools.list_ports.comports()
            port_names = [p.device for p in ports]
            self.port_combo["values"] = port_names

            if port_names:
                if self.port_var.get() not in port_names:
                    self.port_var.set(port_names[0])
            else:
                self.port_var.set("")

            self._log("COM ports refreshed.")
        except Exception as exc:
            self._log(f"Error refreshing COM ports: {exc}")

    def toggle_connection(self):
        if self.is_connected:
            self.disconnect_serial()
        else:
            self.connect_serial()

    def connect_serial(self):
        port = self.port_var.get().strip()
        baud_str = self.baud_var.get().strip()

        if not port:
            messagebox.showwarning("No Port Selected", "Please select a COM port first.")
            return

        try:
            baudrate = int(baud_str)
        except ValueError:
            messagebox.showerror("Invalid Baudrate", "Baudrate must be a valid integer.")
            return

        try:
            self.ser = serial.Serial(
                port=port,
                baudrate=baudrate,
                timeout=0,          # non-blocking
                write_timeout=0
            )
            self.rx_buffer.clear()
            self._set_connected_ui(True)
            self._log(f"Connected to {port} @ {baudrate} baud.")
        except Exception as exc:
            self.ser = None
            self._set_connected_ui(False)
            self._log(f"Connection failed: {exc}")
            messagebox.showerror("Connection Error", str(exc))

    def disconnect_serial(self):
        try:
            if self.ser and self.ser.is_open:
                self.ser.close()
        except Exception as exc:
            self._log(f"Error while disconnecting: {exc}")
        finally:
            self.ser = None
            self.rx_buffer.clear()
            self._set_connected_ui(False)
            self._log("Disconnected from serial port.")

    def safe_serial_failure(self, exc: Exception):
        """Called if cable is unplugged or serial suddenly fails."""
        self._log(f"Serial error: {exc}")
        self.disconnect_serial()

    # ==========================================================
    # Frame helpers
    # ==========================================================
    @staticmethod
    def build_frame(cmd: int, payload: bytes = b"") -> bytes:
        length = len(payload)
        cs = cmd ^ length
        for b in payload:
            cs ^= b
        return bytes([0xAA, cmd, length]) + payload + bytes([cs])

    def send_enroll_request(self):
        """
        Based on your protocol and App.c:
        exact frame = 0xAA 0x10 0x00 0x10
        """
        if not (self.ser and self.ser.is_open):
            self._log("Cannot send enrollment request: serial not connected.")
            return

        frame = self.build_frame(self.CMD_ENROLL_REQ, b"")

        try:
            self.ser.write(frame)
            self._log("TX -> Enrollment request sent: AA 10 00 10")
        except Exception as exc:
            self.safe_serial_failure(exc)

    # ==========================================================
    # Non-blocking poll loop
    # ==========================================================
    def poll_serial(self):
        """
        Called every 50 ms using root.after().
        Non-blocking.
        """
        try:
            if self.ser and self.ser.is_open:
                waiting = self.ser.in_waiting
                if waiting > 0:
                    incoming = self.ser.read(waiting)
                    if incoming:
                        self.rx_buffer.extend(incoming)
                        self.parse_rx_buffer()
        except Exception as exc:
            self.safe_serial_failure(exc)
        finally:
            self.root.after(self.POLL_INTERVAL_MS, self.poll_serial)

    # ==========================================================
    # Robust parser
    # ==========================================================
    def parse_rx_buffer(self):
        """
        Parser behavior:
        - Scan for SOF 0xAA
        - Ensure at least header is available
        - Read LEN
        - Wait until full frame arrives
        - Validate XOR checksum
        - Drop corrupted frames cleanly
        """
        while True:
            # Need at least SOF + CMD + LEN + CS => minimum 4 bytes
            if len(self.rx_buffer) < 4:
                return

            # Scan for SOF
            if self.rx_buffer[0] != self.SOF:
                sof_index = self.rx_buffer.find(bytes([self.SOF]))
                if sof_index == -1:
                    # No SOF at all, drop everything
                    self.rx_buffer.clear()
                    return
                else:
                    # Drop garbage before SOF
                    del self.rx_buffer[:sof_index]

                if len(self.rx_buffer) < 4:
                    return

            # Now rx_buffer[0] should be SOF
            cmd = self.rx_buffer[1]
            length = self.rx_buffer[2]
            frame_len = 4 + length  # SOF + CMD + LEN + DATA + CS

            # Wait for full frame
            if len(self.rx_buffer) < frame_len:
                return

            frame = self.rx_buffer[:frame_len]
            del self.rx_buffer[:frame_len]

            # Validate checksum
            data = frame[3:3 + length]
            received_cs = frame[-1]

            calc_cs = cmd ^ length
            for b in data:
                calc_cs ^= b

            if calc_cs != received_cs:
                self._log(
                    f"RX checksum error -> frame dropped "
                    f"(CMD=0x{cmd:02X}, LEN={length}, RX_CS=0x{received_cs:02X}, CALC_CS=0x{calc_cs:02X})"
                )
                continue

            # Valid frame
            self.handle_frame(cmd, bytes(data))

    # ==========================================================
    # Frame handling
    # ==========================================================
    def handle_frame(self, cmd: int, data: bytes):
        if cmd == self.CMD_ENROLL_ST:
            self.handle_enroll_status(data)

        elif cmd == self.CMD_LOG_ACCESS:
            self.handle_access_log(data)

        else:
            self._log(f"RX unknown command: 0x{cmd:02X} | Data: {data.hex(' ')}")

    def handle_enroll_status(self, data: bytes):
        # 1. Change expected length from 1 to 3
        if len(data) != 3:
            self._log(f"Invalid 0x11 frame length: expected 3, got {len(data)}")
            return

        instruction_id = data[0]
        self._update_banner(instruction_id)

        text, _ = self.INSTRUCTION_MAP.get(
            instruction_id,
            (f"UNKNOWN INSTRUCTION: {instruction_id}", "#7f8c8d")
        )

        # 2. Check if it's the Success instruction (Index 5 in your INSTRUCTION_MAP)
        if instruction_id == 5:
            # 3. Extract the 16-bit User ID from bytes 1 and 2
            user_id = (data[1] << 8) | data[2]
            
            # Append the ID to the text string
            text = f"{text} (Assigned ID: {user_id})"
            
            # Update the big top banner so the Admin sees the ID instantly
            self.banner_label.configure(text=text)

        # Print to the scrolling log
        self._log(f"RX 0x11 -> {text}")
    def handle_access_log(self, data: bytes):
        """
        Command 0x12 payload format:
        Status (1B), UserID (2B), Hours (1B), Minutes (1B), Seconds (1B),
        Day (1B), Month (1B), Year (2B)

        Big-endian:
            >B H B B B B B H
        """
        if len(data) != 10:
            self._log(f"Invalid 0x12 frame length: expected 10, got {len(data)}")
            return

        try:
            status, user_id, hh, mm, ss, day, month, year = struct.unpack(">B H B B B B B H", data)

            if status == 1:
                emoji = "✅"
                access_text = "GRANTED"
                user_text = f"User ID: {user_id}"
            else:
                emoji = "❌"
                access_text = "DENIED"
                user_text = "User ID: 0"

            log_line = (
                f"{emoji} "
                f"{day:02d}/{month:02d}/{year:04d} "
                f"{hh:02d}:{mm:02d}:{ss:02d} | "
                f"{user_text} | Access: {access_text}"
            )

            self._log(log_line)

        except struct.error as exc:
            self._log(f"Struct unpack error for 0x12 payload: {exc}")

    # ==========================================================
    # Close handler
    # ==========================================================
    def on_close(self):
        self.disconnect_serial()
        self.root.destroy()


def main():
    root = tk.Tk()

    # ttk theme
    try:
        style = ttk.Style()
        if "clam" in style.theme_names():
            style.theme_use("clam")
    except Exception:
        pass

    app = FingerprintAccessGUI(root)
    root.mainloop()


if __name__ == "__main__":
    main()