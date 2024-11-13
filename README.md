# ProtoESP
### _Protogen ESP32 controller for MAX7219 / WS2812 Matrix_
**ESP32-S3** code which makes your protogen alive with animations and interactivity!  
*Not 100% complete, might be buggy, refer to  [Legacy](../Legacy) branch.*

##### Main features
- Utilizes **MAX7219** matrixes using SPI or **[WS2812B RGB Matrixes](https://foxxo.cz/proto/matrix/)** to show faces/animations
- Has two additional WS2812 outputs:
	1. for ring LEDs in the ears (animated or coded);
	2. for leds under the eyes (can be repurposed for something else)
- Provides a WiFi AP with **site to control** your protogen (choose animation; change color,brightness,tilt anims,triggers...)
	- Has copy of an **Animator** , so you can make, edit or test the animations on the fly. Frame by frame, pixel by pixel.
- You can change your animations/faces either by:
	1. Cycling them using a button from the ESP;
	2. Choosing on the WiFi site on a phone;
	3. Using a **wireless remote** with buttons
- & much more with these parts:

##### Connect and get more features from:
- **KY-032**: IR Sensor for changing to different animation if booped
- **MAX9814**: Microphone to move protogens mouth (not used to change voice!)
- **LSM6DS3**: Accelerometer to change animations by tilting your head
- **SSD1306**: OLED display to show current animation, speaking status, current&voltage (INA219) and more
- **INA219**: Measure current and voltage of the protogen helmet
- **XIAO ESP32**: Change animations remotely without wiring a single cable from the helmet
- **PWM fan**: Connect speed signal to a 4 pin fan and control its speed

Sponsored by: <a href="https://pcbway.com/g/77jC58"><img src="https://www.electronics-lab.com/wp-content/uploads/2020/04/0x0.png" height="40"></a>

![espprotopromo](https://github.com/user-attachments/assets/c8a63f7a-2e5f-45b6-80ef-6bb01c3a7538)

## Navigation
- [**Controller**](ProtoESP-Controller) .md: ProtoESP controller code with all features
- [**Remote**](ProtoESP-Remote) .md: ProtoESP *(Not legacy)*  Wireless control remote
- [**Legacy**](../Legacy) branch: Legacy controller&remote&WS2812 matrix <- all three not supported anymore but **stable&working**
- [**Matrix**](https://foxxo.cz/proto/matrix/) site: WS2812B 8x8 RGB Matrix replacement for single color MAX7219 Matrixes
- [**Animator**](https://foxxo.cz/proto/animator.html) site: Program to make animations for the controller
- [**ToDo**](todo.md) .md: Checklist of To Do things

## Documentation
- How to DIY protogen: [Imgur](https://imgur.com/a/jYpSbuZ)
- Parts list: [Pastebin](https://pastebin.com/7z4fnVfQ)
- Connection diagram: TBD
- Program/flash manual: TBD
- Remote how to: TBD
- IR Sensor setup: TBD
- Alive ProtoESP protogens / creators: [Furo](https://instagram.com/proto_furo), [Arkoss](https://www.instagram.com/snowkatark/), [Jura](https://www.instagram.com/jura_furr/), [BFoxCZ](https://www.instagram.com/bfoxcz/), & more...

### Feature request / issues
- If you have found an issue with this code (crashes, something not working like it should), please open a issue in this repository!
- If you have thought of a new feature or QoL improvement you would like to see being implemented, please contact me directly on Discord or Telegram: @NCPlyn.

## Support
If you have found this project helpful / used it / want to support and encourage me into working more, you can do so here: [PayPal](https://paypal.me/NCPlyn) or [Revolut](https://revolut.me/ncplyn).
Any amount is more than welcome! Don't forget to add contact info or DM me, so I can add you here:

- **<a href="https://pcbway.com/g/77jC58">PCBWay</a>** has sponsored this project by providing prototype PCBs!
- [**Arkoss**](https://www.instagram.com/snowkatark/), Thank you for your donation!
- [**Alellv**](https://github.com/Alellv), Thank you for your donation!

## Use / license
- GPL 3.0 code license applies
- If you use this code, try to make your protogen unique and change the animations up a little bit in the Animator!
- Sharing and small credit won't hurt right?
