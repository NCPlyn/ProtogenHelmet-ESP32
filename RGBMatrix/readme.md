# RGBMatrix
![MatrixPhoto](https://foxxo.cz/proto/protomatrix.jpg)

RGB version of the very popular LED matrix. Made because 8x8 single-color MAX7219 matrices are limiting and there wasn't a good RGB-capable option in the same size, so this one was designed to fill that gap.

## Production files
The Gerber files needed for production are in the **`v3`** and **`v4`** folders, ready to be sent to PCBWay or JLCPCB for panelization/manufacture.

You need to have the PCBs themselves made (panelize in 2x3 grid) and also order **PCB ASSEMBLY** with the BOM and PnP files to have the LEDs mounted/soldered on! (The cost of one working matrix is **4,5-8€, NOT 0,5€**!)

#### Do not test your luck
Both v3 and v4 have diodes that can fail with inproper reflow temperatures, so select these assembly settings for JLCPCB:
- Surface Finish: **LeadFree HASL**
- Solder Paste: **Medium temp.** (not lead free / 200+°C, THE MAIN THING)
- Bake Components: **Yes** (bake as the respective Worldsemi datasheet describes — WS2812B-MINI-V3-W for v3, WS2812B-2020 for v4)

#### Recommended heatsink (v3 & v4)
Both versions run hot at higher brightness, so a 20x20mm heatsink on the back is advised for either version: [LINK](https://vi.aliexpress.com/item/1005003241466529.html)

---

## v3 — WS2812B-Mini-V3
**32mm 8x8 RGB Matrix — Version 3.1**

- 64x 3535 SMD **RGB** WS2812B-Mini-V3 LEDs
- 4 Layer **black** PCB
- 32mm x 32mm x 3.6mm
- Input and outputs on **all** sides to daisy chain
- Voltage: **5V**
- Working temperature: 40-60°C

**Current draw per matrix:**
- Idle: 35mA
- 100% brightness, 64 LEDs RED: 766mA
- 100% brightness, 64 LEDs GREEN: 383mA
- 100% brightness, 64 LEDs BLUE: 785mA
- 50% brightness, 1/3 LEDs RED: 198mA (normal usage)

**Photos:**

<div style="display:flex">
<img src="https://foxxo.cz/proto/matrix/img/P1150447.jpg" alt="v3" height="400">
<img src="https://foxxo.cz/proto/matrix/img/p20250209.jpg" alt="v3" height="400">
</div>
<div style="display:flex">
<img src="https://foxxo.cz/proto/matrix/img/P1150450.jpg" alt="v3" height="400">
<img src="https://foxxo.cz/proto/matrix/img/IMG112712.jpg" alt="v3" height="400">
</div>

> Last photo above is a prototype showing how to connect the matrices; the third photo is the up-to-date version.

- Working proof: [YouTube](https://youtu.be/rTwX0en7kYo)

---

## v4 — WS2812B-2020
A cheaper, simplified take on the matrix.

- Uses **WS2812B-2020** package LEDs instead
- **2 layer** PCB instead of 4 layers
- ⚠️ Data input/output traces run along the **borders** of the board, so take care when handling, soldering, or daisy-chaining near the edges not to damage them
- LEDs have a **square** look (no domed lens), so printing a small shroud/diffuser to shape each diode is recommended for a nicer finished look

**Photos:**
<div style="display:flex">
<img src="https://foxxo.cz/proto/matrix/img/matrixv4front.jpg" alt="v4 front" height="400">
<img src="https://foxxo.cz/proto/matrix/img/matrixv4back.jpg" alt="v4 back" height="400">
</div>

---

#### Legacy version
The original version with the older 2020-package WS2812 (used on the original prototype) can be found [here](https://github.com/NCPlyn/ProtogenHelmet-ESP32/tree/Legacy/RGB-Matrix).

---
 
## Usage / manufacturing terms
These Gerber files are free to use for your own personal projects. This includes:
- Ordering a manufacture for yourself (e.g. via JLCPCB/PCBWay) for your own personal use
- A builder manufacturing and using these boards in a project (e.g. a Protogen build) that was commissioned by someone, and selling/delivering that finished project to the person who commissioned it

**Not allowed:** manufacturing these PCBs (assembled or bare) to sell as a standalone product/part. If you want to resell these as boards/matrices on their own, please contact me first.
