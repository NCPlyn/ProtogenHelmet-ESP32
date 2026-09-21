#include "devices.h"
#include <vector>
#include <Arduino.h>

MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, MAX_MOSI, MAX_CLK, MAX_CS, MATRIXESNUM);

ezButton hwBtn(animBtn);

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

const std::vector<std::vector<int>> lookupDiag1 = 
{{14,15,16},
 {13,27,28,1},
 {12,26,36,17,2},
 {25,35,29,18},
 {11,34,37,30,3},
 {24,33,31,19},
 {10,23,32,20,4},
 {9,22,21,5},
 {8,7,6}};

const std::vector<std::vector<int>> lookupDiag2 = 
{{2,3,4},
 {1,18,19,5},
 {16,17,30,20,6},
 {28,29,31,21},
 {15,36,37,32,7},
 {27,35,33,22},
 {14,26,34,23,8},
 {13,25,24,9},
 {12,11,10}};

SSDOLED oled;

LSM6DS3 myIMU;

Adafruit_INA219 ina219;

Adafruit_APDS9960 apds;

Adafruit_VL53L1X vl53;
