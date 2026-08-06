#pragma once
#include "settings.h"
#include <Arduino.h>
#include <ezButton.h>
#include <SparkFunLSM6DS3.h>
#include <Adafruit_INA219.h>
#include "Adafruit_APDS9960.h"
#include "Adafruit_VL53L1X.h"
#include "esp_adc/adc_oneshot.h"
#include <FastLED.h>
#include "oled.h"
#include <Wire.h>
#include <Adafruit_INA219.h> //edited library in this sketch (replace 0.1R with 0.03R resistor on the board)
#include "Adafruit_APDS9960.h"
#include "Adafruit_VL53L1X.h"
#include <ezButton.h>

//--------------------------------//MAX LEDs
#include <MD_MAX72xx.h>
#include <SPI.h>

extern MS_MAX72XX mx;
ezButton hwBtn(animBtn);

//--------------------------------//WS2812 LEDs
extern CRGB earLeds[earLedsNum];
extern CRGB blushLeds[blushLedsNum];
extern CRGB visorLeds[MATRIXESNUM*64];
extern CRGB visorLedsNEW[MATRIXESNUM*64];
extern CRGB c2Leds[earLedsNum+blushLedsNum];

extern CLEDController *ledController[2];

extern CRGB pixelBuffer[18];
extern CRGB visorPixelBuffer[10];
extern uint8_t noiseData[earLedsNum];

DEFINE_GRADIENT_PALETTE( blackWhite_gp ) {
  0,   100,  0,   0,
  120,   0,  0,   0,
  255, 255,  255, 255
};
extern CRGBPalette16 blackWhite = blackWhite_gp;

extern const std::vector<std::vector<int>> lookupDiag1;

extern const std::vector<std::vector<int>> lookupDiag2;

extern SSDOLED oled;

extern LSM6DS3 myIMU;

extern  Adafruit_INA219 ina219;

extern Adafruit_APDS9960 apds;

extern Adafruit_VL53L1X vl53;
