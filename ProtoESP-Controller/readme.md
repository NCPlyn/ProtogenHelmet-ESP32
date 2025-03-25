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
- Install if not already: VS Code + PlatformIO extension, Python 3
- Clone this repository, open this folder with PlatformIO and let it download all needed files
- In `src/main.cpp`:
  - Change visorType definition for the display you have (line 22)
  - Change amount of matrices connected for ones you're using (line 23 & 24)
  - Declare if you're using blush LEDs and how many (line 29 & 30)
  - Declare if you're using ear LEDs and how many (line 26 & 27)
  - Change INApresent if you (don't)have INA219 connected (line 32)
- In `platformio.ini`:
  - If using different capacity than n16r8, change to proper sized board (line 2)
  - If your board has lower flash capacity than 8MB, change partition file (line 11)
- Connect the ESP32S3 via COM USB connector and in the PIO tab open `esp32-s3`
- Click on `Upload` and after successful operation expand `Platform` dropdown and click on `Upload filesystem image`
- The ProtoESP should now be fully working and ready to configure if now errors are printed in the `Monitor`

### Use
- Connect to the ProtoESP WiFi AP named `ProtoWiFi` with password `Proto1234`
- Open any web browser and visit site `192.168.4.1` with mobile data turned off
- Change the password and name of the WiFi. You will have to reconnect after restarting the ESP!
- Here you can play any available animation, enable or disable features, configure variables and more!
- Click on `Save` after you're done setting things
- If you need to put the remote to defaults, power on the ESP and hold down the BOOT button for 10s
- For creating your own animations, visit the Animator!