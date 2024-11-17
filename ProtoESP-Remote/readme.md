# ProtoESP - Remote
### Wirelessly control your protogen!
Powered with Seeeduino **XIAO ESP32-S3** over BLE!  
*If using Legacy code, this remote won't work, use [this](../../Legacy/BLE-Remote) one.*<br><br>
![remote_banner](https://github.com/user-attachments/assets/e69433f9-51b3-4ee4-9bc1-57ae101f678d)
Sponsored by: <a href="https://www.pcbway.com/project/share/ProtoESP_Remote_66722ab2.html"><img src="https://www.electronics-lab.com/wp-content/uploads/2020/04/0x0.png" height="40"></a> Who have supplied these PCBs for prototyping! Go check out their cheap and fast PCB / CNC manufacturing!
### Main features
- Automatically tries to connect and reconnect
- 7 inputs for buttons (in future flex sensors?), can be set as:
  - 7 animations, one per input
  - **18** animations, one control input (**short press** changes RGB mode for WS2812 Matrixes, **long press** switches between sets), six inputs for animations
- PCB offers to use XIAO **battery support** for 3.7(4.2v) LiPo batteries with connector and power switch
- WiFi AP with configuration site
  - Set WiFi password and name
  - Choose to which BLE server (ProtoESP controller) should remote connect to
  - Associate animations with sets & buttons
- The **PCB can be broken apart** if you want to extend the buttons into paws etc -> solderable pads remain<br><br>
![remote_pcb](https://github.com/user-attachments/assets/c468b4fe-0240-4ee2-a439-224a187af036)
The Gerber file (ProtoESP-Remote_Gerber_2024-11-13.zip) for manufacturing the PCB is in this folder to use for free.
## How to
### Build
- Parts needed are in the Pastebin parts [list](https://pastebin.com/7z4fnVfQ), get different buttons if you're going to implement them into paws etc...
- Solder parts to the PCB according to the photos/silkscreen on the PCB
  - Buttons are soldered to pins 0,1,2,3,8,9,10 of the XIAO, according GPIO pins are written in the code.
- Bridge the middle pad and GND pad together to ground the buttons, (3.3V pad would supply voltage, not used for now)
- Make sure the battery connector has proper polarity, otherwise swap cables in the battery connector and connect battery.
- Connect antenna supplied with the XIAO and stick it to the back of the PCB, to the same with battery.
### Upload
- Open VS Code (Install if not present on your computer, afterwards install PlatformIO extension)
- Clone this repository and open this folder and let PIO download needed files
- Connect the MCU and in the PIO tab open either XIAO if you're using XIAO ESP32S3 or 4d_systems if using any different/dev board
- Click on 'Upload' and after successful operation expand 'Platform' dropdown and click on 'Upload filesystem image'
- The builtin LED should now start blinking
### Use
- The builtin LED should either blink (trying to connect), or stay lighted up (connected)
- Connect to the ESP WiFi AP named 'ProtoRemote' with password 'Proto123'
- Open any web browser and visit site '*192.168.4.1*' with mobile data turned off
- After 6s the avaiable BLE devices should be filled out in the drop down, select which one is your ProtoESP controller (BLE name is same as WiFi name you set on the ProtoESP controller)
- Change the password of the remote AP, you can also change the name. You will have to reconnect after restarting the ESP!
- After proper connection to the controller the LED should stay lit and animations should now be available to choose in the dropdowns
- Choose which animation you want for what button. If you want to use the 'sets', choose control button. (Button numbering goes from left to right, row by row)
- Click on 'Save' after you're done setting things
- If you need to put the remote to defaults, power on the ESP and hold down the BOOT button for 10s
- Now a press of the button should change the animation on the protogen to one you set!
