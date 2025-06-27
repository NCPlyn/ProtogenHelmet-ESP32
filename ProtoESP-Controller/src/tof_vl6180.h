// VL6180X Time-of-Flight sensor support for ESP32
// Author: [Your Name]
//
// This module provides initialization and reading functions for the Adafruit VL6180X sensor.
//
// Wiring (default I2C):
//   - SDA: connect to ESP32 SDA (default GPIO 21)
//   - SCL: connect to ESP32 SCL (default GPIO 22)
//   - VIN: 3.3V or 5V
//   - GND: GND

#ifndef TOF_VL6180_H
#define TOF_VL6180_H

#include <Adafruit_VL6180X.h>

namespace TOF_VL6180 {
    extern Adafruit_VL6180X vl;
    bool begin();
    int readDistance();
}

#endif // TOF_VL6180_H
