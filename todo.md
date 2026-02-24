# An *universal* controller for your protogen!
# Beta version
### Done
- [x] PlatformIO based for ease of use (no longer Arduino IDE)
- [x] Use of ESP32-S3 with PSRAM
- [x] Dynamic anim selector instead of dropdown
- [x] Using PSRAM (so far only current anim)
- [x] More user-friendly UI on mobile for controls (could be better, but will do)
- [x] Factory settings when error read and CRC checking
- [x] Online web version of animator ([HERE](https://foxxo.cz/proto/animator.html))
- [x] Fix current animations
- [x] Add function to change anims with button
- [x] Auto gen CRC before filesystem upload
- [x] Make sure ifdef for definitons work (should), add definition for wifiena (done), RGB orders def (wrote comment)
- [x] Support for ESP32 with PSRAM soldered on
- [x] Supported both MAX and WS28xx displays in the same codebase
- [x] Custom amount of displays and their placement
- [x] Redo tilt calibration code
- [x] OLED init bug fix at runtime (now fixed fr)
- [x] ElegantOTA (**192.168.4.1/update**)
- [x] visor / blush brightness fix (ws 0-255, max 0-16), enable/disable blush&ears
- [x] More QOL for animator (credit to https://xantorohara.github.io/led-matrix-editor/, this is kinda built on top xD)
- [x] Remote: Convert to PlatformIO + add ElegantOTA
- [x] Remote: Buttons XIAO + PCB
- [x] GPIO0 hold when boot = factory (after 10s of runtine, hold boot button for 10s)
- [x] Ledc PWM fan control (should work)
- [x] Remote: GPIO0 hold when boot = factory (not tested but should)
- [x] Remote: Different modes of using the buttons (7 anims/6anims+modifier-long change sets-short change rgb mode)
- [x] Remote: Button on remote changes RGB modes of visor and anim sets
- [x] Remote: v2 PCB (hole for antenna cable or internal antenna, different batt placement, rename)
- [x] Make uptodate parts list for most options
- [x] Remote: Create/finish .MD file with: What is it, how to wire, how to connect/edit, photos + sponsor, how to flash
- [x] Remote: ~~Deep~~Light sleep after 10 minutes & no Wifi clients (devided by double when disconnected from server)
- [x] Remote: Current from batt: Powered On: 110mA; DeepSleep: 15.5 uA (0.015mA) == 4.5h with 500mAh battery
- [x] Better custom WS28xx display ([RGB-Matrix](https://github.com/NCPlyn/ProtogenHelmet-ESP32/tree/ProtoESP/RGBMatrix))
- [x] Slightly faster analog read & oled (8.8->2.5/1.3ms ; 10ms -> 1-3ms)
- [x] Proper platform link, board defines, partitions
- [x] Update NimBLE https://github.com/h2zero/NimBLE-Arduino/blob/master/docs/1.x_to2.x_migration_guide.md
- [x] Completely redo OLED: INA V/A;remote connect/set;animation current/speaking/rgb mode
- [x] Put into classes/seperate files
- [x] Per matrix, per frame color for RGB matrices
- [x] Export anims to zip + loader, download/load config from upload .json (same for remote)
- [x] Combined ears&blush leds into one controller/pin - crash with >2 controllers (RMTvsSPIFFS)
- [x] Fade between frames (only RGB matrices) (**should be ok**)
- [x] Controller readme.md including capabilities, oled explain, add capac/ir/tof boop connection, ears/blush connect, etc..
- [x] .md with touch sensor types and their uses
- [x] VL53L1X/APDS9960 TOF support besides the IR KY-032 sensor
- [x] Remote: NimBLE 2.x upgrade
- [x] Remote: ESP32C3 full support, pins/LED/PCB/Sleep
- [x] More fluid/animated dynamic speech (louder = more further)
- [x] /log to see the logs from the start of the ESP
- [x] Per pixel color visor & animator (WS2812)

### ToDo: Controller
- [ ] Dynamic speech update (louder += wider, clean code)
- [ ] New RGB matrix display (SK9822-EC20 nope, different one wire but faster and small pckg or black)

- [ ] Support Adafruit LED Backpack I2C matrixes; define face {"1;x70","2;x70","1;x71",...}
- [ ] More RGB anims/modes (Rainbow from boop:front wave, entire rgb waves when wiggle...)
- [ ] FastLED 3.9.13+... [GitHub-issue](https://github.com/FastLED/FastLED/issues/1894)

### ToDo: Manuals etc.
- [ ] Animator guide + visor configs