#include <FastCRC.h>
#define ARDUINOJSON_USE_DOUBLE 0
#include <ArduinoJson.h>
#define CONFIG_LITTLEFS_SPIFFS_COMPAT 1
#include <LittleFS.h>
#include <StreamUtils.h>
#include <ESPAsyncWebServer.h>

//--------------------------------//CRC checksum class
class CrcWriter {
public:
  CrcWriter() {
    _hash = _hasher.crc32(NULL, 0);
  }
  size_t write(uint8_t c) {
    _hash = _hasher.crc32_upd(&c, 1);
    return 1;
  }
  size_t write(const uint8_t *buffer, size_t length) {
    _hash = _hasher.crc32_upd(buffer, length);
    return length;
  }
  uint32_t hash() const {
    return _hash;
  }
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
  bool getBool(AsyncWebServerRequest *req, const char *name, bool &out);
  bool getInt(AsyncWebServerRequest *req, const char *name, int &out);
  bool getFloat(AsyncWebServerRequest *req, const char *name, float &out);
  bool getString(AsyncWebServerRequest *req, const char *name, String &out);
};

