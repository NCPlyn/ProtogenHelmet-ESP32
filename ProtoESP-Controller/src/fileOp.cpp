#include "fileOp.h"

//--------------------------------//CRC checksum class
CrcWriter::CrcWriter() {
  _hash = _hasher.crc32(NULL, 0);
}

size_t CrcWriter::write(uint8_t c) {
  _hash = _hasher.crc32_upd(&c, 1);
  return 1;
}

size_t CrcWriter::write(const uint8_t *buffer, size_t length) {
  _hash = _hasher.crc32_upd(buffer, length);
  return length;
}

uint32_t CrcWriter::hash() const {
  return _hash;
}

//--------------------------------//Config variables store/save/load/default
void Config::setDefault() {
  boopEna = true;
  speechEna = true;
  tiltEna = false;
  bleEna = false;
  oledEna = false;
  bEar = 60;
  bVisor = 100;
  bBlush = 100;
  bOled = 2;
  rbSpeed = 15;
  rbWidth = 8;
  spMin = 90;
  spMax = 110;
  spTrig = 1500;
  aTilt = "confused.json";
  aUp = "sad.json";
  neutralX = 0.3;
  neutralY = 0.92;
  neutralZ = 0.33;
  tiltX = 0.05;
  tiltY = 0.7;
  tiltZ = 0.73;
  upX = -0.17;
  upY = 0.99;
  upZ = 0.11;
  tiltTol = 0.1;
  visColorStr = "#ff0000";
  visColor = strtol(visColorStr.c_str()+1, NULL, 16);
  wifiName = "Proto";
  wifiPass = "Proto123";
  ledType = "WS2812";
  fanDuty = 255;
  save();
}

bool Config::save() {
  JsonDocument doc;

  File file = LittleFS.open("/config.json", "w");
  if (!file) {
    Serial.println("[E] There was an error opening the config file!");
    file.close();
    return false;
  }
  Serial.println("[I] Config file opened to save!");

  doc["boopEna"] = boopEna;
  doc["speechEna"] = speechEna;
  doc["tiltEna"] = tiltEna;
  doc["bleEna"] = bleEna;
  doc["oledEna"] = oledEna;
  doc["bEar"] = bEar;
  doc["bVisor"] = bVisor;
  doc["bBlush"] = bBlush;
  doc["bOled"] = bOled;
  doc["rbSpeed"] = rbSpeed;
  doc["rbWidth"] = rbWidth;
  doc["spMin"] = spMin;
  doc["spMax"] = spMax;
  doc["spTrig"] = spTrig;
  doc["aTilt"] = aTilt;
  doc["aUp"] = aUp;
  doc["neutralX"] = neutralX;
  doc["neutralY"] = neutralY;
  doc["neutralZ"] = neutralZ;
  doc["tiltX"] = tiltX;
  doc["tiltY"] = tiltY;
  doc["tiltZ"] = tiltZ;
  doc["upX"] = upX;
  doc["upY"] = upY;
  doc["upZ"] = upZ;
  doc["tiltTol"] = tiltTol;
  doc["visColor"] = visColorStr;
  doc["wifiName"] = wifiName;
  doc["wifiPass"] = wifiPass;
  doc["ledType"] = ledType;
  doc["fanDuty"] = fanDuty;
  
  CrcWriter CRCchk;
  serializeJson(doc, CRCchk);
  Serial.println("[I] Config CRC is: "+ String(CRCchk.hash()));
  File fileCRC = LittleFS.open("/configCRC.txt", "w");
  if(!fileCRC) {
    Serial.println("[E] There was an error opening the config CRC file!");
    fileCRC.close();
  } else {
    fileCRC.print(String(CRCchk.hash()));
    fileCRC.close();
    Serial.println("[I] CRC saved");
  }

  if(serializeJson(doc, file) == 0) {
    Serial.println("[E] Failed to deserialize the config file");
    return false;
  }

  file.close();
  Serial.println("[I] Config file saved with CRC");

  return true;
}

bool Config::load() {
  JsonDocument doc;

  File file = LittleFS.open("/config.json", "r");
  if (!file) {
    Serial.println("[E] There was an error opening the config file, using default config");
    //using default
    return false;
  }
  Serial.println("[I] Config file opened");

  if(deserializeJson(doc, file)){
    Serial.println("[E] Failed to deserialize the config file, using default config");
    //using default
    return false;
  }
  file.close();
  Serial.println("[I] Deserialized JSON");
  
  CrcWriter CRCchk;
  serializeJson(doc, CRCchk);
  File fileCRC = LittleFS.open("/configCRC.txt", "r");
  if(!fileCRC) {
    Serial.println("[E] There was an error opening the config CRC file, using default config");
    fileCRC.close();
    return false;
  } else {
    String jsonCRC;
    while(fileCRC.available()) {
      jsonCRC+=char(fileCRC.read());
    }
    fileCRC.close();
    Serial.println("[I] CRC is: " + String(CRCchk.hash()) + " and should be: " + jsonCRC);
    if(jsonCRC == String(CRCchk.hash())) {
      Serial.println("[I] CRC check OK");
    } else {
      Serial.println("[E] CRC not the same, using default config");
      return false;
    }
  }
  
  boopEna = doc["boopEna"].as<bool>();
  speechEna = doc["speechEna"].as<bool>();
  tiltEna = doc["tiltEna"].as<bool>();
  bleEna = doc["bleEna"].as<bool>();
  oledEna = doc["oledEna"].as<bool>();
  bEar = doc["bEar"].as<int>();
  bVisor = doc["bVisor"].as<int>();
  bBlush = doc["bBlush"].as<int>();
  bOled = doc["bOled"].as<int>();
  rbSpeed = doc["rbSpeed"].as<int>();
  rbWidth = doc["rbWidth"].as<int>();
  spMin = doc["spMin"].as<int>();
  spMax = doc["spMax"].as<int>();
  spTrig = doc["spTrig"].as<int>();
  aTilt = doc["aTilt"].as<String>();
  aUp = doc["aUp"].as<String>();
  neutralX = doc["neutralX"].as<float>();
  neutralY = doc["neutralY"].as<float>();
  neutralZ = doc["neutralZ"].as<float>();
  tiltX = doc["tiltX"].as<float>();
  tiltY = doc["tiltY"].as<float>();
  tiltZ = doc["tiltZ"].as<float>();
  upX = doc["upX"].as<float>();
  upY = doc["upY"].as<float>();
  upZ = doc["upZ"].as<float>();
  tiltTol = doc["tiltTol"].as<float>();
  visColorStr = doc["visColor"].as<String>();
  visColor = strtol(visColorStr.c_str()+1, NULL, 16);
  wifiName = doc["wifiName"].as<String>();
  wifiPass = doc["wifiPass"].as<String>();
  ledType = doc["ledType"].as<String>();
  fanDuty = doc["fanDuty"].as<int>();

  return true;
}