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
- [x] Proper partition tables for current microcontrollers
- [x] Auto gen CRC before filesystem upload
- [x] Make sure ifdef for definitons work (should), add definition for wifiena (done), RGB orders def (wrote comment)
- [x] Support for ESP32 with PSRAM soldered on
- [x] Supported both MAX and WS28xx displays in the same codebase (**should work but needs to be kept under supervision**)
- [x] Custom amount of displays and their placement (**same as above**)
- [x] Redo tilt calibration code
- [x] OLED init bug fix at runtime (now fixed fr)
- [x] ElegantOTA (**192.168.4.1/update**)
- [x] visor / blush brightness fix (ws 0-255, max 0-16)
- [x] More QOL for animator (credit to https://xantorohara.github.io/led-matrix-editor/, this is kinda built on top xD)
- [x] Remote: Convert to PlatformIO + add ElegantOTA
- [x] Remote: Buttons XIAO + PCB
- [x] GPIO0 hold when boot = factory (after 10s of runtine, hold boot button for 10s)
- [x] Ledc PWM fan control (should work)
- [x] Remote: GPIO0 hold when boot = factory (not tested but should)
- [x] Remote: Different modes of using the buttons (7 anims/6anims+modifier-long change sets-short change rgb mode)
- [x] Remote: Button on remote changes RGB modes of visor and anim sets
- [x] Remote: v2 PCB (hole for antenna cable or internal antenna, different batt placement, rename)
- [x] Make uptodate parts list for most options (**add apds later on if works**)
- [x] Remote: Create/finish .MD file with: What is it, how to wire, how to connect/edit, photos + sponsor, how to flash
- [x] Remote: ~~Deep~~Light sleep after 10 minutes & no Wifi clients (devided by double when disconnected from server)
- [x] Remote: Current from batt: Powered On: 110mA; DeepSleep: 15.5 uA (0.015mA) == 4.5h with 500mAh battery
- [x] Better custom WS28xx display (**In stock** [RGB-Matrix](https://foxxo.cz/proto/matrix/))
- [x] Slightly faster analog read & oled (8.8->2.5/1.3ms ; 10ms -> 1-3ms)
- [x] Proper platform link, board defines, partitions
- [x] Update NimBLE https://github.com/h2zero/NimBLE-Arduino/blob/master/docs/1.x_to2.x_migration_guide.md (should work)
- [x] Completely redo OLED: INA V/A;remote connect/set;animation current/speaking/rgb mode

### ToDo: Controller
- [ ] Debug and get FastLED 3.9.13+ working (glitching AF, test on ESP32 + ESP32S3)
- [ ] Fade between frames, add frame color/per matrix color
- [ ] More RGB anims/modes (Rainbow from boop:front wave, entire rgb waves when wiggle...)

- [ ] Optimize execution speed: xTask?
- [ ] Serial.println() ->> ESP_LOG(I/V/W/D) https://docs.espressif.com/projects/esp-idf/en/v5.4/esp32/api-reference/system/log.html ????
- [ ] VL6180/APDS9960 TOF support besides the ([IR](http://irsensor.wizecode.com/)) sensor
- [ ] Ear/blush disable?
- [ ] Proper enable/disable of features (code wise or reset) - bug/missed comments code check
- [ ] Put into classes/seperate files (slowly doing it)
- [ ] Apa102 style led matrix? (With clock and 20x20 IC size)
- [ ] Support Adafruit LED Backpack I2C matrixes; define face {"1;x70","2;x70","1;x71",...}

### ToDo: Remote
- [ ] Update NimBLE
- [ ] ESP-C3/C6 support, proper platform link, proper board def

### ToDo: Manuals etc.
- [ ] Make "How to properly place and tune IR proximity sensor" for boop
- [ ] Controller readme.md including capabilities, how to flash, explain head definiton for animation/upgrade, etc...
- [ ] Is Remote readme.md uptodate? Sleep, OTA, long 0 reset etc....