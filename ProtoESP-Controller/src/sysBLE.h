#pragma once
//--------------------------------//BLE
#define CONFIG_BT_NIMBLE_MAX_CONNECTIONS 2
#define CONFIG_BT_NIMBLE_ROLE_CENTRAL_DISABLED
#define CONFIG_BT_NIMBLE_ROLE_OBSERVER_DISABLED
#define CONFIG_BT_NIMBLE_MEM_ALLOC_MODE_EXTERNAL 1
#include "NimBLEDevice.h"

extern BLEServer *pServer = NULL;
extern BLECharacteristic * pCharacteristic;
extern BLEAdvertising* pAdvertising;

extern class MyCallbacks: public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
      String temp = String(pCharacteristic->getValue().c_str());
      if(temp.charAt(0) == 'g') { //legacy remote reasons
        pCharacteristic->setValue("i"+String(totalAnims));
        pCharacteristic->notify();
      } else if (temp.charAt(0) == '?') {
        String animtemp;
        for(int i = 0; i < totalAnims; i++) {
          animtemp += availAnims[i].substring(0, availAnims[i].length() - 5);
          animtemp += ";";
        }
        pCharacteristic->setValue(animtemp);
        pCharacteristic->notify(true);
      } else if (temp.charAt(0) == ';') { //command
        if (temp.indexOf("rgb") > 0 && visorType == "WS2812") {
          visorNow->type++;
          if(visorNow->type == visTypeSize)
            visorNow->type = 0;
          if(cfg.oledEna && oledInitDone)
            oled.writeRGB(vTAcro[visorNow->type]);
        } else if (temp.indexOf("set") > 0) {
          temp.remove(0,4);
          if(cfg.oledEna && oledInitDone)
            oled.writeSet(temp.toInt()+1);
        }
      } else if (temp.toInt() > 0 && temp.toInt() <= totalAnims){ //legacy remote reasons
        animToLoad = availAnims[temp.toInt()-1];
      } else {
        for(int i = 0; i < totalAnims; i++) {
          if(temp == availAnims[i].substring(0, availAnims[i].length() - 5)) {
            animToLoad = availAnims[i];
          }
        }
      }
      logPrint("[I] BT Recv.: "+temp);
    };
} chrCallbacks;

extern class ServerCallbacks : public NimBLEServerCallbacks {
  void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
      NimBLEDevice::startAdvertising();
  }
} serverCallbacks;

bool startBLE();
