# Smart Room Automation System (ESP32)

Smart Room Automation is an ESP32-based IoT project that controls room lighting using occupancy detection, ambient light sensing, local button input, and Blynk remote control.

This repository contains the firmware used by Team 7, IIIT Dharwad (AY 2025-2026), for an intelligent single-room lighting automation prototype focused on energy efficiency and practical user control.

## Features

- Occupancy estimation using HC-SR04 ultrasonic trend analysis (entry/exit direction logic)
- Ambient light-aware switching using an LDR
- Relay-based light control (5V single-channel relay)
- Local push-button control:
	- Short press toggles light manually
	- Long press toggles Study Mode
- Remote control via Blynk (ESP32 Wi-Fi)
- Real-time status display on 16x2 I2C LCD
- Serial debug logs at 115200 baud
- Median filtering + cooldown-based event suppression for stable sensing

## System Architecture

The firmware follows a layered control model:

- Input Layer:
	- HC-SR04 ultrasonic sensor (motion direction trend)
	- LDR (ambient light)
	- Push button (manual actions)
	- Blynk virtual pins (remote mode/light actions)
- Decision Layer:
	- ESP32 evaluates sensor trends, occupancy count, and mode states
- Output Layer:
	- Relay drives room light ON/OFF
- Feedback Layer:
	- 16x2 I2C LCD (occupancy, distance, light, mode)
	- Serial Monitor logs for debugging

## Hardware Used

- ESP32 Dev Board
- HC-SR04 Ultrasonic Sensor
- LDR + resistor divider (analog input)
- 5V Single Channel Relay Module
- 16x2 I2C LCD (address: `0x27`)
- Push button (with pull-up logic)
- Jumper wires + power supply

### Electrical Safety Note

For HC-SR04 with ESP32, Echo is 5V and ESP32 GPIO is 3.3V tolerant. Use a voltage divider (example from report: $2k\Omega + 1k\Omega$) on Echo to protect the ESP32 input pin.

## Pin Mapping (Current Firmware)

From [main.ino](main.ino):

- `TRIG_PIN`: GPIO 5
- `ECHO_PIN`: GPIO 18
- `RELAY_PIN`: GPIO 4
- `BUTTON_PIN`: GPIO 14
- `LDR_PIN`: GPIO 34

## Control Logic

### 1. Occupancy Trend Detection

Instead of a PIR trigger, the system uses a rolling distance history:

- Strictly increasing trend -> entry event
- Strictly decreasing trend -> exit event

To improve reliability:

- Distance is median-filtered from multiple HC-SR04 samples
- Invalid distances are discarded
- A cooldown interval prevents repeated triggers

### 2. Lighting Decision

- On entry, occupancy increments and light turns ON only if ambient light is dark (`LDR < DARK_THRESHOLD`)
- On exit, occupancy decrements and is clamped to non-negative values
- If occupancy reaches 0 and Study Mode is OFF, light turns OFF

### 3. Mode Priority (Implemented Behavior)

Current code behavior supports these control states:

1. Manual action (button short press / Blynk actions)
2. Study Mode (forces ON unless Sleep Mode is enabled)
3. Sleep Mode (forces light OFF while active)
4. Normal sensor-based control

Note: Automatic NTP-based sleep scheduling is described in the report as a target behavior, but the present firmware controls Sleep Mode via Blynk input (`V2`) rather than time sync logic.

## Blynk Interface (Current Code)

- `V0`: Study Mode toggle
- `V2`: Sleep Mode toggle

When Sleep Mode is turned ON via Blynk, light is forced OFF.

## LCD Output

Main screen format:

- Line 1: `P:<count> D:<distance>cm`
- Line 2: `L:<ON/OFF> <mode>`

Mode labels:

- `SLP`: Sleep Mode
- `STD`: Study Mode
- `NRM`: Normal Mode

Animations:

- `>> ENTER >>` when entry is detected
- `<< EXIT <<` when exit is detected

## Serial Monitor

Baud rate: `115200`

Used for:

- Startup/debug tracing
- Entry/exit detection logs
- Occupancy count tracking

## Software Dependencies

Install these Arduino libraries:

- `WiFi.h` (ESP32 core)
- `BlynkSimpleEsp32.h` (Blynk)
- `Wire.h`
- `LiquidCrystal_I2C.h`

## Setup Instructions

1. Install Arduino IDE (or PlatformIO) with ESP32 board support.
2. Install required libraries (Blynk + I2C LCD).
3. Open [main.ino](main.ino).
4. Update credentials and tokens in the sketch:
	 - `BLYNK_TEMPLATE_ID`
	 - `BLYNK_TEMPLATE_NAME`
	 - `BLYNK_AUTH_TOKEN`
	 - `ssid`
	 - `pass`
5. Connect hardware as per pin mapping.
6. Upload to ESP32 and open Serial Monitor at `115200`.
7. Verify LCD status updates and Blynk control.

## Important Security Note

Do not commit real Wi-Fi passwords or Blynk auth tokens to public repositories. Use placeholders in committed code and keep actual credentials private.

## Known Challenges (From Project Report)

- Sensor voltage compatibility required Echo line protection
- Initial power distribution issues caused unstable behavior
- Mode conflict handling required tighter control precedence

## Future Roadmap

- DS3231 RTC integration for offline timekeeping
- NTP/RTC-backed scheduled Sleep Mode
- FreeRTOS task-based restructuring for responsiveness
- Voice assistant integration (Alexa/Google Home)
- OLED-based richer local UI
- Multi-room scalability and energy analytics

## Repository Structure

- [main.ino](main.ino): Complete ESP32 firmware
- [README.md](README.md): Project documentation

## Team

Team 7, IIIT Dharwad
Academic Year 2025-2026