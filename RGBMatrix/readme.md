# RGBMatrix
![MatrixPhoto](https://foxxo.cz/proto/protomatrix.jpg)
### Information about the matrix and if you can order them from me atm: [foxxo.cz/proto/matrix](https://foxxo.cz/proto/matrix/)
- The needed files for production are in this repository folder, ready to be sent to PCBWay or JLCPCB.
- **DO NOT TEST YOUR LUCK**
  - Add these information to your order:
    - Surface Finish: **LeadFree HASL**
    - Solder Paste: **Medium temp.** (not lead free / 200+°C)
    - Bake Components: **Yes (Bake as the Worldsemi-WS2812B-MINI-V3-W datasheet describes.)**
    - PCBA remark: **Please follow the reflow stated by the Worldsemi-WS2812B-MINI-V3-W datasheet or the SMT paste.**
  - To be even more sure, have the manufacturer test them for you with "SP002E LED controller" so they will replace the faulty LEDs if any show up
    - Tell them to check for completely dead LEDs or any single color of each LEDS (red,green,blue sub led), the whole matrix should be light up

#### New version with `SK9822` that should be better in development
Older version with 2020 package WS2812 [here](https://github.com/NCPlyn/ProtogenHelmet-ESP32/tree/Legacy/RGB-Matrix) (These are used on my own proto)
