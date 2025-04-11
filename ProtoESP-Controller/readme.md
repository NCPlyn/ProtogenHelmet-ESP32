# ProtoESP - Controler
### The working brain of your Protogen!
![IMG_20230228_191415](https://github.com/user-attachments/assets/cc3951e1-8a25-4073-93dd-ec07dff64e9a)
### Main features
- TBD
## How to
### Build
- **Connect matrices in following order** (start at the right eye when your head is in the helmet)
![image](https://github.com/user-attachments/assets/c1ca67f9-de79-4f47-be82-14fa918654f9)

- **Connect all available components according this schematic** (visor leds & power supply are the ones completely necessary, other are optional but preffered)
![ProtoESP-Controller_SCHEM](controller-diagram.png)

### Upload
- Install if not already: VS Code + PlatformIO extension and Python 3
- Clone this repository, open this folder with PlatformIO and let it download all needed files
- In `src/main.cpp`:
  - Change `visorType` definition for the display you have (WS2812 or MAX72XX)
  - Change `MAX72xx_DEVICES` or `visorLedsNum` to the correct amount of matrices/LEDs connected
  - Change `FADESTEPS` to the amount of steps to fade between frames (0 to disable - good for frames with <100ms)
  - Change `earPresent` if you're or not using ear LEDs
  - Change `blushPresent` if you're or not using blush LEDs (set `useRGBblush` to true if they are RGB and not GRB)
  - Change `INApresent` if you have or not INA219 connected
  - Change `boopMode` to what you use as boop sensor (`IR-KY` for KY032/active LOW or `Capac` for capacitive/active HIGH)
- In `platformio.ini`:
  - If using different capacity than n16r8, change to proper sized board (line 2)
  - If your board has lower flash capacity than 8MB, change partition file (line 11)
- Connect the ESP32S3 via COM USB connector and in the PIO tab open `esp32-s3`
- Click on `Upload` and after successful operation expand `Platform` dropdown and click on `Upload filesystem image`
- If `configCRC.txt` doesn't get automaticaly generated (`Generated CRC` in the terminal) while building the filesystem, run "genCRC_manual.py" manually and then `Upload filesystem image` again
- The ProtoESP should now be fully working and ready to configure if now errors are printed in the `Monitor`

### Use
- Connect to the ProtoESP WiFi AP named `ProtoWiFi` with password `Proto1234`
- Open any web browser and visit site `192.168.4.1` with mobile data turned off
- Change the password and name of the WiFi. You will have to reconnect after restarting the ESP!
- Here you can play any available animation, enable or disable features, configure variables and more!
- Click on `Save` after you're done setting things
- If you need to put the remote to defaults, power on the ESP and hold down the BOOT button for 10s
- For creating your own animations, visit the Animator!