"""
STM32 Fingerprint Access Control GUI - LinkedIn Demo Version
------------------------------------------------------------

A polished Tkinter + PySerial desktop GUI for presenting an STM32-based
fingerprint biometric access control system.

Requirements:
    pip install pyserial

UART Frame Format:
    [SOF 0xAA] [CMD] [LEN] [DATA...] [CHECKSUM]

Checksum:
    CMD ^ LEN ^ DATA[0] ^ ... ^ DATA[N-1]

Commands:
    0x10 : PC -> STM32  : Start enrollment
    0x11 : STM32 -> PC  : GUI/enrollment instruction
    0x12 : STM32 -> PC  : Access log frame
    0x13 : STM32 -> PC  : Power/Sleep state
"""

import struct
import tkinter as tk
from tkinter import ttk, messagebox
import serial
import serial.tools.list_ports


class FingerprintAccessGUI:
    # ==========================================================
    # Protocol constants
    # ==========================================================
    SOF = 0xAA

    CMD_ENROLL_REQ = 0x10
    CMD_ENROLL_ST = 0x11
    CMD_LOG_ACCESS = 0x12
    CMD_SLEEP_ST = 0x13  # NEW: Power state command

    POLL_INTERVAL_MS = 50

    INSTRUCTION_MAP = {
        0: ("System Idle", "Place your finger on the sensor to unlock the door", "#1E3A8A"),
        1: ("Place Finger", "Enrollment step 1: place your finger on the sensor", "#F59E0B"),
        2: ("Lift Finger", "Remove your finger from the sensor", "#EA580C"),
        3: ("Place Again", "Enrollment step 2: place the same finger again", "#F59E0B"),
        4: ("Processing", "Fingerprint module is processing the image", "#0EA5E9"),
        5: ("Enrollment Successful", "Fingerprint template stored successfully", "#16A34A"),
        6: ("Enrollment Failed", "The fingerprint could not be enrolled", "#DC2626"),
        7: ("Bad Scan", "Scan quality was poor. Please try again", "#EF4444"),
        8: ("System Asleep", "STM32 is in low-power standby mode. UART paused.", "#0F172A"), # NEW: Sleep State
    }

    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("STM32 Fingerprint Access Control Dashboard")
        self.root.geometry("1120x720")
        self.root.minsize(980, 640)
        self.root.configure(bg="#64748B")

        # Serial state
        self.ser = None
        self.is_connected = False
        self.rx_buffer = bytearray()

        # GUI variables
        self.port_var = tk.StringVar()
        self.baud_var = tk.StringVar(value="9600")
        self.connection_status_var = tk.StringVar(value="DISCONNECTED")
        self.current_state_var = tk.StringVar(value="System Idle")
        self.current_details_var = tk.StringVar(value="Place your finger on the sensor to unlock the door")
        self.last_user_var = tk.StringVar(value="--")
        self.last_access_var = tk.StringVar(value="--")
        self.last_time_var = tk.StringVar(value="--")

        # Blinking state tracking
        self.blink_state = True
        self.blink_job = None
        self.reset_job = None  # Tracks the auto-reset timer

        self._setup_styles()
        self._build_gui()

        self.refresh_ports()
        self._set_connected_ui(False)
        self._update_status_card(0)

        self.root.after(self.POLL_INTERVAL_MS, self.poll_serial)
        self.root.protocol("WM_DELETE_WINDOW", self.on_close)

    # ==========================================================
    # Styling
    # ==========================================================
    def _setup_styles(self):
        self.style = ttk.Style()
        try:
            self.style.theme_use("clam")
        except Exception:
            pass

        self.style.configure(
            "Modern.TCombobox",
            fieldbackground="#FFFFFF",
            background="#FFFFFF",
            foreground="#263244",
            padding=8,
            arrowsize=18,
            font=("Segoe UI", 11)
        )

        self.style.configure(
            "Log.Treeview",
            background="#263244",
            foreground="#F8FAFC",
            fieldbackground="#263244",
            rowheight=32,
            borderwidth=0,
            font=("Segoe UI", 10)
        )

        self.style.configure(
            "Log.Treeview.Heading",
            background="#66778C",
            foreground="#FFFFFF",
            font=("Segoe UI", 10, "bold"),
            padding=8
        )

        self.style.map(
            "Log.Treeview",
            background=[("selected", "#2563EB")],
            foreground=[("selected", "#FFFFFF")]
        )

    def _make_button(self, parent, text, command, bg, fg="white", height=2):
        btn = tk.Button(
            parent,
            text=text,
            command=command,
            bg=bg,
            fg=fg,
            activebackground=self._shade(bg, -25),
            activeforeground=fg,
            disabledforeground="#F8FAFC",
            font=("Segoe UI", 12, "bold"),
            relief="raised",
            bd=4,
            cursor="hand2",
            height=height,
            padx=14,
            pady=8
        )

        btn.bind("<Enter>", lambda e: btn.config(bg=self._shade(bg, 18)) if str(btn["state"]) != "disabled" else None)
        btn.bind("<Leave>", lambda e: btn.config(bg=bg) if str(btn["state"]) != "disabled" else None)

        return btn

    @staticmethod
    def _shade(hex_color: str, amount: int) -> str:
        hex_color = hex_color.lstrip("#")
        r = max(0, min(255, int(hex_color[0:2], 16) + amount))
        g = max(0, min(255, int(hex_color[2:4], 16) + amount))
        b = max(0, min(255, int(hex_color[4:6], 16) + amount))
        return f"#{r:02x}{g:02x}{b:02x}"

    # ==========================================================
    # GUI Layout
    # ==========================================================
    def _build_gui(self):
        main = tk.Frame(self.root, bg="#64748B")
        main.pack(fill="both", expand=True, padx=18, pady=18)

        header = tk.Frame(main, bg="#64748B")
        header.pack(fill="x", pady=(0, 16))

        tk.Label(
            header,
            text="STM32 Fingerprint Access Control",
            bg="#64748B",
            fg="#F8FAFC",
            font=("Segoe UI", 24, "bold")
        ).pack(side="left")

        tk.Label(
            header,
            text="Python GUI  •  UART Protocol  •  Biometric Access",
            bg="#64748B",
            fg="#E2E8F0",
            font=("Segoe UI", 11)
        ).pack(side="right", pady=(10, 0))

        body = tk.Frame(main, bg="#64748B")
        body.pack(fill="both", expand=True)

        self.left_panel = tk.Frame(body, bg="#66778C", width=300)
        self.left_panel.pack(side="left", fill="y", padx=(0, 16))
        self.left_panel.pack_propagate(False)

        self.right_panel = tk.Frame(body, bg="#64748B")
        self.right_panel.pack(side="left", fill="both", expand=True)

        self._build_left_panel()
        self._build_right_panel()

    def _build_left_panel(self):
        top_card = tk.Frame(self.left_panel, bg="#3A4B5F", highlightthickness=1, highlightbackground="#64748B")
        top_card.pack(fill="x", padx=16, pady=16)

        tk.Label(
            top_card,
            text="🔐",
            bg="#3A4B5F",
            fg="#FFFFFF",
            font=("Segoe UI Emoji", 34)
        ).pack(pady=(18, 4))

        tk.Label(
            top_card,
            text="Biometric Access GUI",
            bg="#3A4B5F",
            fg="#F8FAFC",
            font=("Segoe UI", 15, "bold")
        ).pack()

        tk.Label(
            top_card,
            text="Control and monitor STM32\nfingerprint access events",
            bg="#3A4B5F",
            fg="#E2E8F0",
            font=("Segoe UI", 10),
            justify="center"
        ).pack(pady=(6, 18))

        form = tk.Frame(self.left_panel, bg="#66778C")
        form.pack(fill="x", padx=16, pady=(4, 12))

        tk.Label(
            form,
            text="CONNECTION",
            bg="#66778C",
            fg="#38BDF8",
            font=("Segoe UI", 10, "bold")
        ).pack(anchor="w", pady=(0, 8))

        tk.Label(form, text="COM Port", bg="#66778C", fg="#F8FAFC", font=("Segoe UI", 10, "bold")).pack(anchor="w")
        self.port_combo = ttk.Combobox(
            form,
            textvariable=self.port_var,
            state="readonly",
            style="Modern.TCombobox",
            width=27
        )
        self.port_combo.pack(fill="x", pady=(4, 10), ipady=5)

        tk.Label(form, text="Baudrate", bg="#66778C", fg="#F8FAFC", font=("Segoe UI", 10, "bold")).pack(anchor="w")
        self.baud_combo = ttk.Combobox(
            form,
            textvariable=self.baud_var,
            values=["9600", "19200", "38400", "57600", "115200", "230400"],
            state="readonly",
            style="Modern.TCombobox",
            width=27
        )
        self.baud_combo.pack(fill="x", pady=(4, 12), ipady=5)

        self.refresh_btn = self._make_button(form, "↻  REFRESH PORTS", self.refresh_ports, "#66778C")
        self.refresh_btn.pack(fill="x", pady=(0, 10))

        self.connect_btn = self._make_button(form, "●  CONNECT", self.toggle_connection, "#16A34A")
        self.connect_btn.pack(fill="x", pady=(0, 16))

        self.status_pill = tk.Label(
            form,
            textvariable=self.connection_status_var,
            bg="#7F1D1D",
            fg="#FFFFFF",
            font=("Segoe UI", 12, "bold"),
            padx=12,
            pady=10
        )
        self.status_pill.pack(fill="x", pady=(0, 18))

        tk.Label(
            form,
            text="ACTIONS",
            bg="#66778C",
            fg="#38BDF8",
            font=("Segoe UI", 10, "bold")
        ).pack(anchor="w", pady=(0, 8))

        self.enroll_btn = self._make_button(form, "🟦  START ENROLLMENT", self.send_enroll_request, "#2563EB", height=3)
        self.enroll_btn.pack(fill="x", pady=(0, 10))

        self.demo_btn = self._make_button(form, "▶  DEMO ACCESS EVENT", self.demo_access_event, "#7C3AED")
        self.demo_btn.pack(fill="x", pady=(0, 10))

        proto = tk.Frame(self.left_panel, bg="#64748B", highlightthickness=1, highlightbackground="#64748B")
        proto.pack(side="bottom", fill="x", padx=16, pady=16)

        tk.Label(
            proto,
            text="UART FRAME",
            bg="#64748B",
            fg="#38BDF8",
            font=("Segoe UI", 9, "bold")
        ).pack(anchor="w", padx=12, pady=(12, 3))

        tk.Label(
            proto,
            text="AA | CMD | LEN | DATA | CS",
            bg="#64748B",
            fg="#F8FAFC",
            font=("Consolas", 11, "bold")
        ).pack(anchor="w", padx=12)

        tk.Label(
            proto,
            text="Checksum = CMD ^ LEN ^ DATA",
            bg="#64748B",
            fg="#E2E8F0",
            font=("Segoe UI", 9)
        ).pack(anchor="w", padx=12, pady=(4, 12))

    def _build_right_panel(self):
        self.status_card = tk.Frame(
            self.right_panel,
            bg="#1E3A8A",
            highlightthickness=0
        )
        self.status_card.pack(fill="x", pady=(0, 14))

        self.state_lbl_header = tk.Label(
            self.status_card,
            text="CURRENT SYSTEM STATE",
            bg="#1E3A8A",
            fg="#DBEAFE",
            font=("Segoe UI", 10, "bold")
        )
        self.state_lbl_header.pack(anchor="w", padx=24, pady=(22, 2))

        self.state_title = tk.Label(
            self.status_card,
            textvariable=self.current_state_var,
            bg="#1E3A8A",
            fg="#FFFFFF",
            font=("Segoe UI", 30, "bold")
        )
        self.state_title.pack(anchor="w", padx=24)

        self.state_details = tk.Label(
            self.status_card,
            textvariable=self.current_details_var,
            bg="#1E3A8A",
            fg="#EFF6FF",
            font=("Segoe UI", 14, "bold")
        )
        self.state_details.pack(anchor="w", padx=24, pady=(4, 22))

        self._build_enrollment_click_section()

        cards = tk.Frame(self.right_panel, bg="#64748B")
        cards.pack(fill="x", pady=(0, 14))

        self._create_info_card(cards, "Last User ID", self.last_user_var, "👤").pack(side="left", fill="x", expand=True, padx=(0, 10))
        self._create_info_card(cards, "Last Access Result", self.last_access_var, "🛡️").pack(side="left", fill="x", expand=True, padx=(0, 10))
        self._create_info_card(cards, "Last Event Time", self.last_time_var, "🕒").pack(side="left", fill="x", expand=True)

        table_card = tk.Frame(self.right_panel, bg="#66778C", highlightthickness=1, highlightbackground="#64748B")
        table_card.pack(fill="both", expand=True)

        table_header = tk.Frame(table_card, bg="#66778C")
        table_header.pack(fill="x", padx=16, pady=(14, 8))

        tk.Label(
            table_header,
            text="Access Events",
            bg="#66778C",
            fg="#F8FAFC",
            font=("Segoe UI", 15, "bold")
        ).pack(side="left")

        tk.Label(
            table_header,
            text="Live logs received from STM32",
            bg="#66778C",
            fg="#E2E8F0",
            font=("Segoe UI", 10)
        ).pack(side="right", pady=(5, 0))

        columns = ("time", "date", "user", "result", "raw")
        self.event_table = ttk.Treeview(
            table_card,
            columns=columns,
            show="headings",
            style="Log.Treeview"
        )

        self.event_table.heading("time", text="Time")
        self.event_table.heading("date", text="Date")
        self.event_table.heading("user", text="User ID")
        self.event_table.heading("result", text="Access")
        self.event_table.heading("raw", text="Message")

        self.event_table.column("time", width=90, anchor="center")
        self.event_table.column("date", width=105, anchor="center")
        self.event_table.column("user", width=90, anchor="center")
        self.event_table.column("result", width=120, anchor="center")
        self.event_table.column("raw", width=380, anchor="w")

        self.event_table.pack(fill="both", expand=True, padx=16, pady=(0, 10))

        console_frame = tk.Frame(table_card, bg="#3A4B5F", highlightthickness=1, highlightbackground="#64748B")
        console_frame.pack(fill="x", padx=16, pady=(0, 16))

        self.console = tk.Text(
            console_frame,
            height=5,
            bg="#3A4B5F",
            fg="#F8FAFC",
            insertbackground="#FFFFFF",
            relief="flat",
            font=("Consolas", 10),
            wrap="word"
        )
        self.console.pack(fill="x", padx=10, pady=8)
        self.console.config(state="disabled")

        self._log("Application started.")
        self._log("Select COM port, choose baudrate, then press CONNECT.")

    def _build_enrollment_click_section(self):
        bg_color = "#1E293B"  
        bd_color = "#38BDF8"  
        
        section = tk.Frame(
            self.right_panel,
            bg=bg_color,
            highlightthickness=1,
            highlightbackground=bd_color
        )
        section.pack(fill="x", pady=(0, 14))

        left = tk.Frame(section, bg=bg_color)
        left.pack(side="left", fill="both", expand=True, padx=18, pady=14)

        tk.Label(
            left,
            text="Enrollment Control",
            bg=bg_color,
            fg="#38BDF8",  
            font=("Segoe UI", 15, "bold")
        ).pack(anchor="w")

        tk.Label(
            left,
            text="Click to send the enrollment command to STM32 and follow the live instruction banner above.",
            bg=bg_color,
            fg="#F8FAFC",  
            font=("Segoe UI", 10),
            wraplength=520,
            justify="left"
        ).pack(anchor="w", pady=(4, 0))

        tk.Label(
            left,
            text="TX frame: AA 10 00 10",
            bg=bg_color,
            fg="#38BDF8",
            font=("Consolas", 11, "bold")
        ).pack(anchor="w", pady=(8, 0))

        right = tk.Frame(section, bg=bg_color)
        right.pack(side="right", padx=18, pady=14)

        self.enrollment_section_btn = self._make_button(
            right,
            "🟦  START ENROLLMENT",
            self.send_enroll_request,
            "#2563EB",
            height=2
        )
        self.enrollment_section_btn.pack(fill="x")

    def _create_info_card(self, parent, title, variable, icon):
        if title == "Last User ID":
            card_bg = "#EAB308"        
            border_color = "#CA8A04"   
            title_fg = "#FEF9C3"       
            value_fg = "#000000"       

        elif title == "Last Access Result":
            card_bg = "#7C3AED"        
            border_color = "#A855F7"
            title_fg = "#F3E8FF"
            value_fg = "#FFFFFF"

        elif title == "Last Event Time":
            card_bg = "#D97706"        
            border_color = "#F59E0B"
            title_fg = "#FEF3C7"
            value_fg = "#FFFFFF"

        else:
            card_bg = "#66778C"
            border_color = "#64748B"
            title_fg = "#E2E8F0"
            value_fg = "#F8FAFC"

        card = tk.Frame(parent, bg=card_bg, highlightthickness=1, highlightbackground=border_color)

        icon_label = tk.Label(
            card,
            text=icon,
            bg=card_bg,
            fg=value_fg,
            font=("Segoe UI Emoji", 22)
        )
        icon_label.pack(anchor="w", padx=18, pady=(14, 0))

        title_label = tk.Label(
            card,
            text=title,
            bg=card_bg,
            fg=title_fg,
            font=("Segoe UI", 10, "bold")
        )
        title_label.pack(anchor="w", padx=18, pady=(4, 0))

        value_label = tk.Label(
            card,
            textvariable=variable,
            bg=card_bg,
            fg=value_fg,
            font=("Segoe UI", 18, "bold")
        )
        value_label.pack(anchor="w", padx=18, pady=(2, 16))

        if title == "Last Access Result":
            self.last_access_card = card
            self.last_access_card_widgets = [card, icon_label, title_label, value_label]

        return card

    # ==========================================================
    # GUI helpers
    # ==========================================================
    def _log(self, message: str):
        self.console.config(state="normal")
        self.console.insert("end", message + "\n")
        self.console.see("end")
        self.console.config(state="disabled")

    def _update_status_card(self, instruction_id: int):
        # Cancel any pending auto-reset timers when a new state arrives
        if self.reset_job is not None:
            self.root.after_cancel(self.reset_job)
            self.reset_job = None

        title, details, color = self.INSTRUCTION_MAP.get(
            instruction_id,
            (f"Unknown Instruction {instruction_id}", "Received an unknown status from STM32", "#64748B")
        )

        self.current_state_var.set(title)
        self.current_details_var.set(details)

        self.status_card.config(bg=color)
        for widget in self.status_card.winfo_children():
            widget.config(bg=color)

        if instruction_id == 0:
            self._start_glow_pulse()
        else:
            self._stop_glow_pulse()

        # If enrollment is successful (ID 5) or failed (ID 6), trigger a 3-second auto-idle reset
        if instruction_id in (5, 6):
            self.reset_job = self.root.after(3000, lambda: self._update_status_card(0))

    def _start_glow_pulse(self):
        """Initializes the background banner pulse loop for high screen visibility."""
        if self.blink_job is not None:
            return
        self.blink_state = True
        self._toggle_glow_effect()

    def _stop_glow_pulse(self):
        """Halts the banner animation loop and restores layout styling presets."""
        if self.blink_job is not None:
            self.root.after_cancel(self.blink_job)
            self.blink_job = None
        try:
            self.status_card.config(bg="#1E3A8A")
            self.state_lbl_header.config(bg="#1E3A8A", fg="#DBEAFE")
            self.state_title.config(bg="#1E3A8A", fg="#FFFFFF")
            self.state_details.config(bg="#1E3A8A", fg="#EFF6FF")
        except Exception:
            pass

    def _toggle_glow_effect(self):
        """Alternates between highly contrasting color sets for maximum visibility."""
        if self.current_state_var.get() == "System Idle":
            if self.blink_state:
                bg_color = "#1E3A8A"
                header_text = "#DBEAFE"
                title_text = "#FFFFFF"
                details_text = "#EFF6FF"
            else:
                bg_color = "#F59E0B"
                header_text = "#78350F"
                title_text = "#0F172A"
                details_text = "#1E293B"

            try:
                self.status_card.config(bg=bg_color)
                self.state_lbl_header.config(bg=bg_color, fg=header_text)
                self.state_title.config(bg=bg_color, fg=title_text)
                self.state_details.config(bg=bg_color, fg=details_text)
            except Exception:
                pass

            self.blink_state = not self.blink_state
            self.blink_job = self.root.after(600, self._toggle_glow_effect)
        else:
            self.blink_job = None

    def _update_last_access_card_color(self, result: str):
        result = str(result).upper().strip()

        if result == "GRANTED":
            bg_color = "#16A34A"
            border_color = "#22C55E"
            title_color = "#DCFCE7"
            value_color = "#FFFFFF"

        elif result == "DENIED":
            bg_color = "#DC2626"
            border_color = "#EF4444"
            title_color = "#FEE2E2"
            value_color = "#FFFFFF"

        else:
            bg_color = "#581C87"
            border_color = "#A855F7"
            title_color = "#F3E8FF"
            value_color = "#FFFFFF"

        try:
            self.last_access_card.config(bg=bg_color, highlightbackground=border_color)

            for widget in self.last_access_card_widgets:
                widget.config(bg=bg_color)

            self.last_access_card_widgets[1].config(fg=value_color)
            self.last_access_card_widgets[2].config(fg=title_color)
            self.last_access_card_widgets[3].config(fg=value_color)

        except Exception as exc:
            self._log(f"Last Access Result colour update failed: {exc}")

    def _set_connected_ui(self, connected: bool):
        self.is_connected = connected

        if connected:
            self.connection_status_var.set("CONNECTED")
            self.status_pill.config(bg="#14532D")
            self.connect_btn.config(text="●  DISCONNECT", bg="#DC2626", activebackground="#B91C1C")
            self.enroll_btn.config(state="normal", bg="#2563EB")
            if hasattr(self, "enrollment_section_btn"):
                self.enrollment_section_btn.config(state="normal", bg="#2563EB")
            self.port_combo.config(state="disabled")
            self.baud_combo.config(state="disabled")
        else:
            self.connection_status_var.set("DISCONNECTED")
            self.status_pill.config(bg="#7F1D1D")
            self.connect_btn.config(text="●  CONNECT", bg="#16A34A", activebackground="#15803D")
            self.enroll_btn.config(state="disabled", bg="#64748B")
            if hasattr(self, "enrollment_section_btn"):
                self.enrollment_section_btn.config(state="disabled", bg="#64748B")
            self.port_combo.config(state="readonly")
            self.baud_combo.config(state="readonly")
            self._update_last_access_card_color("")
            self._update_status_card(0)

    # ==========================================================
    # Serial connection
    # ==========================================================
    def refresh_ports(self):
        try:
            ports = serial.tools.list_ports.comports()
            port_names = [p.device for p in ports]

            self.port_combo["values"] = port_names

            if port_names:
                if self.port_var.get() not in port_names:
                    self.port_var.set(port_names[0])
                self._log(f"COM ports refreshed: {', '.join(port_names)}")
            else:
                self.port_var.set("")
                self._log("No COM ports found. Connect USB-to-UART and press REFRESH PORTS.")

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
            messagebox.showwarning("No COM Port", "Please select a COM port first.")
            return

        try:
            baudrate = int(baud_str)
        except ValueError:
            messagebox.showerror("Invalid Baudrate", "Baudrate must be a number.")
            return

        try:
            self.ser = serial.Serial(
                port=port,
                baudrate=baudrate,
                timeout=0,
                write_timeout=0
            )

            try:
                self.ser.reset_input_buffer()
                self.ser.reset_output_buffer()
            except Exception:
                pass

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
            self._log(f"Disconnect error: {exc}")
        finally:
            self.ser = None
            self.rx_buffer.clear()
            self._set_connected_ui(False)
            self._log("Disconnected from serial port.")

    def safe_serial_failure(self, exc: Exception):
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
        if not (self.ser and self.ser.is_open):
            self._log("Cannot start enrollment: serial is not connected.")
            return

        frame = self.build_frame(self.CMD_ENROLL_REQ, b"")

        try:
            self.ser.write(frame)
            self._log("TX -> Enrollment request sent: AA 10 00 10")
            self._update_status_card(1)
        except Exception as exc:
            self.safe_serial_failure(exc)

    # ==========================================================
    # Non-blocking serial polling
    # ==========================================================
    def poll_serial(self):
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

    def parse_rx_buffer(self):
        while True:
            if len(self.rx_buffer) < 4:
                return

            if self.rx_buffer[0] != self.SOF:
                sof_index = self.rx_buffer.find(bytes([self.SOF]))

                if sof_index == -1:
                    self.rx_buffer.clear()
                    return

                del self.rx_buffer[:sof_index]

                if len(self.rx_buffer) < 4:
                    return

            cmd = self.rx_buffer[1]
            length = self.rx_buffer[2]
            frame_len = 4 + length

            if len(self.rx_buffer) < frame_len:
                return

            frame = self.rx_buffer[:frame_len]
            del self.rx_buffer[:frame_len]

            data = frame[3:3 + length]
            received_cs = frame[-1]

            calc_cs = cmd ^ length
            for b in data:
                calc_cs ^= b

            if calc_cs != received_cs:
                self._log(
                    f"RX checksum error: CMD=0x{cmd:02X}, LEN={length}, "
                    f"RX_CS=0x{received_cs:02X}, CALC_CS=0x{calc_cs:02X}"
                )
                continue

            self.handle_frame(cmd, bytes(data))

    # ==========================================================
    # Frame handling
    # ==========================================================
    def handle_frame(self, cmd: int, data: bytes):
        if cmd == self.CMD_ENROLL_ST:
            self.handle_enroll_status(data)

        elif cmd == self.CMD_LOG_ACCESS:
            self.handle_access_log(data)

        elif cmd == self.CMD_SLEEP_ST:  # Route the sleep frame
            self.handle_sleep_status(data)

        else:
            self._log(f"RX unknown command: 0x{cmd:02X} | Data: {data.hex(' ')}")

    def handle_enroll_status(self, data: bytes):
        if len(data) not in (1, 3):
            self._log(f"Invalid 0x11 frame length: expected 1 or 3, got {len(data)}")
            return

        instruction_id = data[0]
        self._update_status_card(instruction_id)

        title, details, _ = self.INSTRUCTION_MAP.get(
            instruction_id,
            (f"Unknown Instruction {instruction_id}", "Received unknown instruction", "#64748B")
        )

        if instruction_id == 5 and len(data) == 3:
            user_id = (data[1] << 8) | data[2]
            self.current_details_var.set(f"Fingerprint stored successfully. Assigned User ID: {user_id}")
            self.last_user_var.set(str(user_id))
            self._log(f"RX 0x11 -> {title} | Assigned ID: {user_id}")
        else:
            self._log(f"RX 0x11 -> {title} | {details}")

    def handle_sleep_status(self, data: bytes):
        if len(data) != 1:
            self._log(f"Invalid 0x13 frame length: expected 1, got {len(data)}")
            return

        state = data[0]

        if state == 0x01:  # SYSTEM_SLEEP
            # 1. Update banner using the new Map ID 8 (System Asleep)
            self._update_status_card(8)
            
            # 2. Disable the UART interaction buttons
            self.enroll_btn.config(state="disabled", bg="#64748B")
            self.enrollment_section_btn.config(state="disabled", bg="#64748B")
            
            self._log("RX 0x13 -> System entered SLEEP mode. Interaction disabled.")

        elif state == 0x00:  # SYSTEM_AWAKE
            # 1. Re-enable the UART interaction buttons if still connected
            if self.is_connected:
                self.enroll_btn.config(state="normal", bg="#2563EB")
                self.enrollment_section_btn.config(state="normal", bg="#2563EB")

            # 2. Return the banner to the System Idle state (Map ID 0)
            self._update_status_card(0)
            
            self._log("RX 0x13 -> System WOKE UP. System is now Idle.")

        else:
            self._log(f"RX 0x13 -> Unknown sleep state payload: {state}")

    def handle_access_log(self, data: bytes):
        if len(data) != 10:
            self._log(f"Invalid 0x12 frame length: expected 10, got {len(data)}")
            return

        try:
            status, user_id, hh, mm, ss, day, month, year = struct.unpack(">B H B B B B B H", data)

            result = "GRANTED" if status == 1 else "DENIED"
            display_user = str(user_id) if status == 1 else "0"
            time_text = f"{hh:02d}:{mm:02d}:{ss:02d}"
            date_text = f"{day:02d}/{month:02d}/{year:04d}"
            message = f"User {display_user} access {result}"

            self.last_user_var.set(display_user)
            self.last_access_var.set(result)
            self._update_last_access_card_color(result)
            self.last_time_var.set(time_text)

            self.event_table.insert(
                "",
                0,
                values=(time_text, date_text, display_user, result, message)
            )

            self._log(f"RX 0x12 -> {date_text} {time_text} | User {display_user} | Access {result}")

        except struct.error as exc:
            self._log(f"Struct unpack error for 0x12 payload: {exc}")

    # ==========================================================
    # Demo helper for LinkedIn recording
    # ==========================================================
    def demo_access_event(self):
        self.last_user_var.set("7")
        self.last_access_var.set("GRANTED")
        self._update_last_access_card_color("GRANTED")
        self.last_time_var.set("10:45:12")
        self.current_state_var.set("Access Granted")
        self.current_details_var.set("Demo event: user authenticated successfully")
        self.status_card.config(bg="#16A34A")
        for widget in self.status_card.winfo_children():
            widget.config(bg="#16A34A")

        self.event_table.insert(
            "",
            0,
            values=("10:45:12", "20/05/2026", "7", "GRANTED", "Demo: fingerprint matched successfully")
        )

        self._log("DEMO -> Access granted event added for LinkedIn recording.")

    # ==========================================================
    # Close handler
    # ==========================================================
    def on_close(self):
        if self.reset_job is not None:
            self.root.after_cancel(self.reset_job)
        self.disconnect_serial()
        self.root.destroy()


def main():
    root = tk.Tk()
    app = FingerprintAccessGUI(root)
    root.mainloop()


if __name__ == "__main__":
    main()