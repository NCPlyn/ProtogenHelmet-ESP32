#include "oled.h"


U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0,/* reset=*/ U8X8_PIN_NONE);

//--------------------------------//OLED Brightness
void SSDOLED::oledBright(int level) const
{
        switch(level)
        {
        case 0: //dim
                u8g2.sendF("ca", 0x0d9, (15 << 4) | 0 );
                u8g2.sendF("ca", 0x0db, 0 << 4);
                break;
        case 1: //mid
                u8g2.sendF("ca", 0x0d9, (15 << 4) | 15 );
                u8g2.sendF("ca", 0x0db, 0 << 4);
                break;
        case 2: //normal
                u8g2.sendF("ca", 0x0d9, (15 << 4) | 15 );
                u8g2.sendF("ca", 0x0db, 7 << 4);
                break;
        }       
}

//--------------------------------//OLED Init
bool SSDOLED::init(uint8_t oledAddr, int brightness, bool INA) {
        INAavail = INA;

        Wire.beginTransmission(oledAddr); //check for oled on address 0x3c
        byte error = Wire.endTransmission();
        if(error != 0) return false;

        u8g2.setI2CAddress(oledAddr << 1);

        if(!u8g2.begin()) return false;

        u8g2.setFlipMode(2);
        oledBright(brightness);
        return true;
}

//--------------------------------//Animation name
int SSDOLED::writeAnim(String anim) const {
        u8g2.setDrawColor(0);
        u8g2.drawBox(0, 0, 128, 35);
        u8g2.setDrawColor(1);
        u8g2.setFont(u8g2_font_logisoso28_tr);
        int width = u8g2.getStrWidth(anim.c_str());
        u8g2.drawStr(width >= 128 ? 0 : (128 - width) / 2, 28, anim.c_str());
        u8g2.updateDisplayArea(0, 0, 16, 5);
        return width;
}

//--------------------------------//INA Voltage & Current
void SSDOLED::writeINA(float volt, float amp) const {
        u8g2.setDrawColor(0);
        u8g2.drawBox(0, 32, 128, 13); //0-128 ; 32-45
        u8g2.setDrawColor(1);
        String toShow = String(volt,2)+"V "+String(amp/1000,2)+"A";
        float width = u8g2.getStrWidth(toShow.c_str());
        u8g2.setFont(u8g2_font_t0_22b_tr);
        u8g2.drawStr((128 - width) / 2, 45, toShow.c_str());
        u8g2.updateDisplayArea(0, 4, 16, 2);
}

namespace
{
        struct SymbolLayout
        {
                int x,                  y;
                int width,              height;
                const uint8_t* ON_bits, OFF_bits;
                int area_x,             area_y;
                int area_w,             area_h;

        };
        constexpr SymbolLayout SPK_SML = {
                x       = 45,                   y               = 50,
                width   = spk_width,            height          = spk_height,
                ON_bits = spkONsml_bits,        OFF_bits        = spkOFFsml_bits,
                area_x  = 5,                    area_y          = 6,
                area_w  = 3,                    area_h          = 2
        };
        constexpr SymbolLayout SPK_LRG = {
                x       = 45,                   y               = 36,
                width   = spk_width * 2,        height          = spk_height * 2,
                ON_bits = spkONlrg_bits,        OFF_bits        = spkOFFlrg_bits,
                area_x  = 5,                    area_y          = 4,
                area_w  = 5,                    area_h          = 4
        };
        constexpr SymbolLayout REM_SML = {
                x       = 8,                    y               = 49,
                width   = rem_width,            height          = rem_height,
                ON_bits = remONsml_bits,        OFF_bits        = remOFFsml_bits,
                area_x  = 1,                    area_y          = 6,
                area_w  = 2,                    area_h          = 2
        };
        constexpr SymbolLayout REM_LRG = {
                x       = 2,                    y               = 36,
                width   = rem_width * 2,        height          = rem_height * 2,
                ON_bits = remONlrg_bits,        OFF_bits        = remOFFlrg_bits,
                area_x  = 0,                    area_y          = 4,
                area_w  = 4,                    area_h          = 4
        };

}

//--------------------------------//Speaking symbol
void SSDOLED::speak(bool show) const {
    const SymbolLayout& cfg = INAavail ? SPK_SML : SPK_LRG;
    u8g2.drawXBM(cfg.x, cfg.y, cfg.width, cfg.height, show ? cfg.ON_bits : cfg.OFF_bits);
    u8g2.updateDisplayArea(cfg.area_x, cfg.area_y, cfg.area_w, cfg.area_h);
}

//--------------------------------//Remote symbol
void SSDOLED::remote(bool show) const {
    const SymbolLayout& cfg = INAavail ? REM_SML : REM_LRG;
    u8g2.drawXBM(cfg.x, cfg.y, cfg.width, cfg.height, show ? cfg.ON_bits : cfg.OFF_bits);
    u8g2.updateDisplayArea(cfg.area_x, cfg.area_y, cfg.area_w, cfg.area_h);
}
//--------------------------------//Remote set number
void SSDOLED::writeSet(int setNum) const {
  if(INAavail) {
    u8g2.setDrawColor(0);
    u8g2.drawBox(20, 50, 10, 13); //20-30 ; 50-63
    u8g2.setDrawColor(1);
    u8g2.setFont(u8g2_font_t0_22b_tf);
    u8g2.drawStr(19, 63, String(setNum).c_str());
    u8g2.updateDisplayArea(2, 6, 2, 2);
  } else {
    u8g2.setDrawColor(0);
    u8g2.drawBox(25, 40, 13, 21); //25-38 ; 40-61
    u8g2.setDrawColor(1);
    u8g2.setFont(u8g2_font_helvB18_tn);
    u8g2.drawStr(26, 59, String(setNum).c_str());
    u8g2.updateDisplayArea(3, 5, 2, 3);
  }
}

//--------------------------------//RGB acronym status
void SSDOLED::writeRGB(String name) const {
  u8g2.setFont(u8g2_font_t0_22b_tr);
  if(INAavail) {
    u8g2.setDrawColor(0);
    u8g2.drawBox(69, 49, 53, 15); //69-122 ; 49-64
    u8g2.setDrawColor(1);
    u8g2.drawStr(70, 63, name.c_str());
    u8g2.updateDisplayArea(8, 6, 8, 2);
  } else {
    u8g2.setDrawColor(0);
    u8g2.drawBox(75, 41, 53, 20); //75-128 ; 41-61
    u8g2.setDrawColor(1);
    u8g2.drawStr(76, 55, name.c_str());
    u8g2.updateDisplayArea(9, 5, 7, 3);
  }
}
