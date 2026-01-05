#include <FastCRC.h>
#define ARDUINOJSON_USE_DOUBLE 0
#include <ArduinoJson.h>
#define CONFIG_LITTLEFS_SPIFFS_COMPAT 1
#include <LittleFS.h>
#include <StreamUtils.h>

//--------------------------------//CRC checksum class
class CrcWriter {
public:
  CrcWriter();
  size_t write(uint8_t c);
  size_t write(const uint8_t *buffer, size_t length);
  uint32_t hash() const;
private:
  FastCRC32 _hasher;
  uint32_t _hash;
};

//--------------------------------//Config variables store/save/load/default
class Config {
public:
  bool boopEna, speechEna, tiltEna, bleEna, oledEna;
  int bEar,bVisor,bOled,rbSpeed,rbWidth,spMin,spMax,spTrig,fanDuty,boopThresh;
  float neutralX,neutralY,neutralZ,tiltX,tiltY,tiltZ,upX,upY,upZ,tiltTol;
  String aTilt,aUp,aBoop,visColorStr,wifiName = "ProtoWiFi",wifiPass = "Proto1234";
  unsigned long visColor;
  void setDefault();
  bool save();
  bool load();
};