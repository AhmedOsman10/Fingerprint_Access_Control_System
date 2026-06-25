# 🔐 Fingerprint Access Control System (STM32F407 + FreeRTOS)

An embedded access control system built on the **STM32F407** that combines **fingerprint authentication**, **real-time multitasking**, **low-power operation**, and a **layered firmware architecture**.

The project was designed around three engineering objectives:

- ⚡ **Non-Blocking Execution**
- 🔋 **Power Efficiency**
- 🏗️ **Scalable Software Architecture**

A desktop Python GUI communicates with the STM32 through a custom UART protocol, allowing user enrollment, authentication, and real-time system monitoring.

---

## 📹 Project Demonstration

🎥 **Demo Video:** *(Insert YouTube / LinkedIn Link)*

📂 **GitHub Repository:** *(Insert Repository Link)*

---

## 📖 Project Overview

The system authenticates users using a fingerprint sensor and controls a relay-driven door lock.

Unlike many basic embedded projects, the firmware was designed to demonstrate software engineering techniques commonly used in commercial embedded systems.

### Key Technologies

- FreeRTOS multitasking
- Custom asynchronous UART driver
- Interrupt-driven communication
- Event-driven firmware
- RTC scheduling
- Stop Mode power management
- Layered firmware architecture
- EXTI interrupt handling

### Operating Schedule

| Time | Access |
|------|--------|
| 07:00 – 09:00 | ✅ Enabled |
| 16:00 – 18:00 | ✅ Enabled |
| All Other Times | ❌ Disabled |

Outside these operating windows, the MCU automatically enters **Stop Mode**. The **DS3231 RTC** maintains accurate time while the STM32 internal RTC schedules automatic wake-up events. An EXTI push button provides temporary activation outside normal operating hours.

---

## ✨ Features

### 🔐 Access Control

- Fingerprint authentication
- User enrollment
- User identification
- Relay-controlled door lock

### ⚙️ Embedded Software

- FreeRTOS multitasking
- Custom asynchronous UART driver
- Interrupt-driven communication
- State-machine based enrollment
- Layered firmware architecture

### 🔋 Power Management

- Automatic Stop Mode
- RTC alarm wake-up
- Scheduled operating windows
- EXTI manual activation

### 🖥️ Desktop GUI

- Python GUI
- Real-time enrollment
- Authentication status
- Event monitoring
- UART communication

---

## 🏗️ System Architecture

> **Insert High-Level System Architecture Diagram Here**

---

## 🎯 Engineering Objectives

### ⚡ Non-Blocking Execution

The firmware eliminates blocking delays using **FreeRTOS**, interrupt-driven peripherals, and event-driven task synchronization. A custom asynchronous UART driver allows communication with the desktop GUI while fingerprint authentication, relay control, and scheduling tasks continue executing concurrently.

### 🔋 Power Efficiency

To reduce power consumption, the system operates only during predefined access windows. Outside these periods, the MCU enters **Stop Mode** and is automatically restored by an RTC alarm. A manual EXTI button provides temporary activation when required.

### 🏗️ Scalable Software Architecture

The firmware follows a layered architecture separating application logic from hardware-specific drivers.

- **Application Layer** – Business logic
- **ECUAL Layer** – External device drivers
- **MCAL Layer** – STM32 peripheral drivers

---

## 🏛️ Firmware Architecture

> **Insert Layered Architecture Diagram Here**

### Application Layer

- Access control
- User enrollment
- Authentication
- GUI communication
- Sleep management
- Scheduling

### ECUAL Drivers

- Fingerprint Sensor
- DS3231 RTC
- EEPROM
- Relay
- Push Button

### MCAL Drivers

- RCC
- GPIO
- USART
- Timers
- SysTick
- EXTI

---

## 🧵 FreeRTOS Tasks

| Task | Responsibility |
|------|----------------|
| Fingerprint Task | Enrollment & Authentication |
| Application Task | System Coordination |
| Relay Task | Door Lock Control |

---

## 🔄 System Workflows

### Access Verification

> **Insert Workflow Diagram**

### User Enrollment

> **Insert Workflow Diagram**

### Sleep Mode

> **Insert Workflow Diagram**

### Manual Activation

> **Insert Workflow Diagram**

---

## 🖥️ Desktop GUI

The custom Python GUI provides:

- User enrollment
- Authentication status
- User ID display
- Event logging
- UART communication
- Real-time feedback

> **Insert GUI Screenshots**

---

## 📡 UART Communication Protocol

| Field | Description |
|------|-------------|
| SOF | Start of Frame (0xAA) |
| CMD | Command Identifier |
| LEN | Payload Length |
| DATA | Payload |
| CS | XOR Checksum |

### Example Commands

| Command | Description |
|---------|-------------|
| 0x10 | Enrollment Request |
| 0x11 | Enrollment Status |
| 0x12 | Access Event |

> **Insert UART Protocol Diagram**

---

## 🔌 Hardware

### Main Controller

- STM32F407G-DISC1

### Peripherals

- Fingerprint Sensor
- DS3231 RTC Module
- I²C EEPROM
- ULN2003 Driver
- Relay Module
- Solenoid Lock
- Push Button (EXTI)

> **Insert Hardware Photos**

---

## 🛠️ Development Tools

- STM32CubeIDE
- FreeRTOS
- Python
- Git
- GitHub
- VS Code
- ST-Link
- Logic Analyzer

---

## 📸 Power Management Verification

Power efficiency was verified using a logic analyzer.

The capture demonstrates:

- Normal operation
- Automatic Stop Mode entry
- Approximately 125 seconds of low-power operation
- RTC alarm wake-up
- Automatic restoration of system tasks

> **Insert Logic Analyzer Capture**

---

## 📈 Project Highlights

- ✅ Fingerprint authentication system
- ✅ FreeRTOS multitasking
- ✅ Custom asynchronous UART driver
- ✅ Layered firmware architecture
- ✅ RTC scheduling
- ✅ Stop Mode implementation
- ✅ Python desktop GUI
- ✅ Event-driven application design

---

## 🚀 Future Improvements

- BLE/Wi-Fi connectivity
- Encrypted UART communication
- Configurable access schedules
- SD card event logging
- OTA firmware updates

---

## 👨‍💻 Author

**Ahmed Abdelrhman**

Embedded Systems Engineer

### Areas of Interest

- Embedded Firmware Development
- STM32 Microcontrollers
- FreeRTOS
- Device Driver Development
- Low-Power Embedded Systems
- Industrial Automation

---

⭐ **If you found this project interesting, consider giving the repository a star!**
""")

path="/mnt/data/README.md"
with open(path,"w",encoding="utf-8") as f:
    f.write(content)

print(path)
