// APDS9960 Proximity sensor support for ESP32
// Author: [Your Name]
//
// This module provides initialization and reading functions for the SparkFun APDS9960 sensor.
//
// Wiring (default I2C):
//   - SDA: connect to ESP32 SDA (default GPIO 21)
//   - SCL: connect to ESP32 SCL (default GPIO 22)
//   - VIN: 3.3V
//   - GND: GND

#ifndef TOF_APDS9960_H
#define TOF_APDS9960_H

#include <SparkFun_APDS9960.h>

namespace TOF_APDS9960 {
    extern SparkFun_APDS9960 apds;
    bool begin();
    int readProximity();
}

#endif // TOF_APDS9960_H
