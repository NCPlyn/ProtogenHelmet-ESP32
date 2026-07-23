//Make sure you have everything connected by the schematic in the repository and set these defines correctly!

#define MICpin ADC_CHANNEL_0 //Microphone, pin 1
#define T_in 2 //Output from Touch Sensor (for KY-032, Capac)
#define T_en 42 //Enable pin to Touch Sensor (for KY-032)
#define DATA_PIN_EARS 5  //Ears(Blush) (from outer to inner, POV-right cheek, (if blush: from top to bottom, right cheek nearest to ear first))
#define DATA_PIN_VISOR 7 //Face (right cheek, left segment of eye first)
#define I2C_SDA 8 //SDA for Gyro, OLED, INA219, ToF
#define I2C_SCL 9 //SCL for Gyro, OLED, INA219, ToF
#define MAX_CLK 12 //Clock for MAX72xx matrixes if used
#define MAX_MOSI 11 //Data for MAX72xx matrixes if used
#define MAX_CS 10 //ChipSelect for MAX72xx matrixes if used
#define animBtn 4 //Pulling this pin LOW cycles through animations
#define fanPWM 13 //PWM pin to control 4pin fan

#define visorType "WS2812" // What displays are you using? (WS2812 or MAX72XX so far)
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW //flip up-down: ::DR1CR0RR1_HW , flip left-right: ::PAROLA_HW , flip both: ::ICSTATION_HW
#define MATRIXESNUM 11 // How many matrices for visor? 11
#define FADESTEPS 4 //how many steps when fading between frames? (0=disabled; only for WS2812 displays)

inline bool earPresent = false; // Are you using ear leds?
#define earLedsNum 74 // How many? (74 or 32 rn)

inline bool blushPresent = false; // Are you using blush leds?
#define blushLedsNum 8 // How many? (might crash under 8)
inline bool useRGBblush = true; //Swaps red-green for RGB strip

inline bool INApresent = false; //Are you using INA219?

#define boopMode "APDS9960" //"KY-032" for KY-032, "Capac" for capacitive sensor/boop when HIGH, "APDS9960" for ADPS9960, "VL53L1X" for VL53L1X, leave empty for none

#define revertTilt 8000 //The maximum time that animation caused by tilt gets shown (used as if tilt bugs out etc)

#define oldMatrixFix false //fix for Legacy WS2812B-2020 matrix

#define oledAddr 60 //define oled on address 0x3c

