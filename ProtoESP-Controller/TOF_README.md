# Time-of-Flight (TOF) Sensor Support

This project now supports two additional proximity sensors alongside the existing IR sensor:

## Supported TOF Sensors

### 1. Adafruit VL6180X (Time-of-Flight Distance Sensor)
- **Measures:** Distance in millimeters (mm)
- **Wiring (I2C):**
  - SDA: ESP32 SDA (default GPIO 8)
  - SCL: ESP32 SCL (default GPIO 9)
  - VIN: 3.3V or 5V
  - GND: GND

### 2. SparkFun APDS9960 (Proximity/Color/Gesture Sensor)
- **Measures:** Proximity value (0-255)
- **Wiring (I2C):**
  - SDA: ESP32 SDA (default GPIO 8)
  - SCL: ESP32 SCL (default GPIO 9)
  - VIN: 3.3V
  - GND: GND

## Configuration
- No changes to existing IR sensor logic. All sensors work in parallel.
- Libraries are automatically installed via PlatformIO:
  - `Adafruit VL6180X`
  - `SparkFun APDS9960`

## Usage
- Sensor values are displayed on the OLED (if enabled) and logged to Serial every second.
- Initialization errors are reported to Serial.

## Manual Testing
- Check Serial Monitor for `[TOF] VL6180X Distance: ...` and `[TOF] APDS9960 Proximity: ...` messages.
- On OLED, look for a line like `VL:xxxmm AP:yyy` (where xxx is distance in mm, yyy is proximity value).
- If values are not updating, check wiring and I2C addresses.

## Schematic
See `controller-diagram.png` for I2C wiring reference.

---

*For more details, see the main README.*
