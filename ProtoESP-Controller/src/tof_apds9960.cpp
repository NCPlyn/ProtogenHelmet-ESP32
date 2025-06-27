// APDS9960 Proximity sensor implementation
#include "tof_apds9960.h"
#include <Wire.h>

SparkFun_APDS9960 TOF_APDS9960::apds;

bool TOF_APDS9960::begin() {
    if (!apds.init()) {
        Serial.println("[APDS9960] Failed to initialize!");
        return false;
    }
    if (!apds.enableProximitySensor(false)) {
        Serial.println("[APDS9960] Failed to enable proximity sensor!");
        return false;
    }
    Serial.println("[APDS9960] Proximity sensor initialized.");
    return true;
}

int TOF_APDS9960::readProximity() {
    uint8_t proximity = 0;
    if (!apds.readProximity(proximity)) {
        Serial.println("[APDS9960] Error reading proximity!");
        return -1;
    }
    return (int)proximity;
}
