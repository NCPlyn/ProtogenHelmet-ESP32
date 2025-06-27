// VL6180X Time-of-Flight sensor implementation
#include "tof_vl6180.h"
#include <Wire.h>

Adafruit_VL6180X TOF_VL6180::vl;

bool TOF_VL6180::begin() {
    if (!vl.begin()) {
        Serial.println("[VL6180X] Failed to find sensor!");
        return false;
    }
    Serial.println("[VL6180X] Sensor initialized.");
    return true;
}

int TOF_VL6180::readDistance() {
    int dist = vl.readRange();
    if (vl.timeoutOccurred()) {
        Serial.println("[VL6180X] Timeout occurred!");
        return -1;
    }
    return dist;
}
