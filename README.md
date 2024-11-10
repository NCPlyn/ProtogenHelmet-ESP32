# An *universal* controller for your protogen!
- Beta version
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
- [x] Proper definitions, buggy platformio with defines (should work)
- [x] Auto gen CRC before filesystem upload (platformio is unpredictable)
- [x] Make sure ifdef for definitons work (should), add definition for wifiena (done), RGB orders def (wrote comment)
- [x] Support for ESP32 with PSRAM soldered on / without (**soldered works, without might never be...**)
- [x] Supported both MAX and WS28xx displays in the same codebase (**should work but needs to be kept under supervision**)
- [x] Custom amount of displays and their placement (**same as above**)
- [x] Redo tilt calibration code (**same as above**)
- [x] OLED init bug fix at runtime (now fixed fr)
- [x] ElegantOTA (**192.168.4.1/update**)
- [x] visor / blush brightness fix (ws 0-255, max 0-16)
- [x] More QOL for animator (credit to https://xantorohara.github.io/led-matrix-editor/, this is kinda built on top xD)
- [x] Remote: Convert to PlatformIO + add ElegantOTA
- [x] Remote: Buttons XIAO + PCB
- [x] GPIO0 hold when boot = factory (after 10s of runtine, hold boot button for 10s)
- [x] Ledc PWM fan control (should work)
- [x] Remote: GPIO0 hold when boot = factory (not tested but should)

- ToDo: Helmet
- [ ] VL6180 nope, try APDS9960 -Support for ToF sensor besides the ([IR](http://irsensor.wizecode.com/)) sensor
- [ ] Better custom WS28xx display (**waiting for v3, taking preorders** [RGB-Matrix](https://foxxo.cz/proto/matrix/))
- [ ] Support different sized OLED, setups with INA or not (define addreses, brightness done)
- [ ] Proper enable/disable of features (code wise or reset)
- [ ] More RGB anims/modes (Rainbow from boop/front, etc)
- [ ] Button from remote to change the RGB modes + show on OLED
- [ ] Put into functions/files
- [ ] esp32 spi defines ok??? 2x 18pin? 23 pin? 2x 5 pin esp board, does max really work?

- ToDo: Remote
- [ ] Flex sensors
- [ ] Different modes of using the buttons (7 anims,6anims+modifier-long change sets-short change rgb mode)

- ToDo: Manuals etc.
- [ ] Make "How to properly place and tune proximity sensor" for boop (do not need SW fixing, all is about proper placement and tuning sensor)
- [ ] Make wiring and flashing tutorial (including fs things)
- [ ] Make uptodate parts list for most options
- [ ] Make how to do the remote
  
- But I'm limited in my free time that I can give here so it will take a while!
- If you have found this project helpful or if you have used it and want to support me and encourage me into working more on this project, you can do so here: [PayPal.me](https://paypal.me/NCPlyn)
- Thank you **Arkoss** for your donation! [IG](https://www.instagram.com/snowkatark/)
- Thank you **Alellv** for you donation! [GitHub](https://github.com/Alellv)