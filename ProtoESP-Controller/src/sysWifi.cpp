#include "sysWifi.h"
#include "configVars.h"

//--------------------------------//WiFi server setup
void startWiFiWeb() {
  WiFi.softAP(cfg.wifiName, cfg.wifiPass);

  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

  server.on("/getfiles", HTTP_GET, [](AsyncWebServerRequest *request){ //returns current anim + all avaible anims
    if(getfilesProper) {
      getFilesFunc();
    }
    request->send(200, "text/plain", currentAnim+";"+getfilesCache);
  });

  server.on("/saveconfig", HTTP_GET, [](AsyncWebServerRequest *request){ //saves config
    //features enable
    cfg.getBool(request, "boopEna", cfg.boopEna);
    cfg.getBool(request, "speechEna", cfg.speechEna);
    cfg.getBool(request, "tiltEna", cfg.tiltEna);
    cfg.getBool(request, "bleEna", cfg.bleEna);
    cfg.getBool(request, "oledEna", cfg.oledEna);
    //brightness
    cfg.getInt(request, "bEar", cfg.bEar);
    cfg.getInt(request, "bVisor", cfg.bVisor);
    if(cfg.getInt(request, "bOled", cfg.bOled)) {
        if(cfg.oledEna && oledInitDone)
            oled.oledBright(cfg.bOled);
    }
    //animation settings
    cfg.getInt(request, "rbSpeed", cfg.rbSpeed);
    cfg.getInt(request, "rbWidth", cfg.rbWidth);
    cfg.getInt(request, "spMin", cfg.spMin);
    cfg.getInt(request, "spMax", cfg.spMax);
    cfg.getInt(request, "spTrig", cfg.spTrig);
    //tilt animations
    cfg.getString(request, "aTilt", cfg.aTilt);
    cfg.getString(request, "aUp", cfg.aUp);
    cfg.getString(request, "aBoop", cfg.aBoop);
    //tilt neutral
    cfg.getFloat(request, "neutralX", cfg.neutralX);
    cfg.getFloat(request, "neutralY", cfg.neutralY);
    cfg.getFloat(request, "neutralZ", cfg.neutralZ);
    //tilt triggers
    cfg.getFloat(request, "tiltX", cfg.tiltX);
    cfg.getFloat(request, "tiltY", cfg.tiltY);
    cfg.getFloat(request, "tiltZ", cfg.tiltZ);
    cfg.getFloat(request, "upX", cfg.upX);
    cfg.getFloat(request, "upY", cfg.upY);
    cfg.getFloat(request, "upZ", cfg.upZ);
    cfg.getFloat(request, "tiltTol", cfg.tiltTol);
    //RGB visor color
    if(cfg.getString(request, "visColor", cfg.visColorStr))
        cfg.visColor = strtol(cfg.visColorStr.c_str() + 1, NULL, 16);
    //wifi
    cfg.getString(request, "wifiName", cfg.wifiName);
    cfg.getString(request, "wifiPass", cfg.wifiPass);
    //boop threshold
    cfg.getInt(request, "boopThresh", cfg.boopThresh);
    delay(25);
    if(cfg.save()) {
      instantReload = true;
      request->redirect("/saved.html?main");
    } else {
      request->send(200, "text/plain", F("Saving config failed!"));
    }
  });

  server.on("/savefile", HTTP_POST, [](AsyncWebServerRequest *request){ //saves data from POST to file
    if(request->hasParam("file", true) && request->hasParam("content", true)) {
      File file = LittleFS.open("/anims/"+request->getParam("file", true)->value()+".json", "w");
      if (!file) {
        logPrint(F("[E] There was an error opening the file for saving an animation!"));
        file.close();
        request->send(200, "text/plain", F("Error opening file for writing!"));
      } else {
        logPrint(F("[I] File saved!"));
        file.print(request->getParam("content", true)->value());
        file.close();
        getfilesProper = true;
        request->redirect("/saved.html?anim");
      }
    } else {
      request->send(200, "text/plain", F("No valid parameters detected!"));
    }
  });

  server.on("/deletefile", HTTP_GET, [](AsyncWebServerRequest *request){ //deletes asked file
    if(request->hasParam("file")) {
      LittleFS.remove("/anims/"+request->getParam("file")->value());
      getfilesProper = true;
      request->redirect("/saved.html?main");
    } else {
      request->send(200, "text/plain", F("Parameter 'file' not present!"));
    }
  });

  server.on("/change", HTTP_GET, [](AsyncWebServerRequest *request){ //loads anim from selected avaible anims
    if(request->hasParam("anim")) {
      if(request->getParam("anim")->value() == currentAnim) {
        request->redirect("/saved.html?main");
      } else if(loadAnim(request->getParam("anim")->value(),"")) {
        request->redirect("/saved.html?main");
      } else {
        request->send(200, "text/plain", F("Loading animation has failed!"));
      }
    } else {
      request->send(200, "text/plain", F("No valid parameters detected!"));
    }
  });

  server.on("/change", HTTP_POST, [](AsyncWebServerRequest *request){ //loads anim from POST request
    if(request->hasParam("anim", true)) {
      if(loadAnim("POSTAnimLoad",request->getParam("anim", true)->value())) {
        request->redirect("/saved.html?main");
      } else {
        request->send(200, "text/plain", F("Loading animation has failed!"));
      }
    } else {
      request->send(200, "text/plain", F("No valid parameters detected!"));
    }
  });

  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });

  server.on("/fanpwm", HTTP_GET, [](AsyncWebServerRequest *request){
    if(request->hasParam("duty")) {
      int duty = request->getParam("duty")->value().toInt();
      if(duty < 256 && duty >= 0) {
        //ledcWrite(0, duty);
        ledcWrite(fanPWM, duty); //Arduino 3.x core
        cfg.fanDuty = duty;
        cfg.save();
        request->send(200, "text/plain", "Set PWM to: " + String(duty));
      } else {
        request->send(200, "text/plain", F("Invalid duty cycle"));
      }
    } else {
      request->send(200, "text/plain", F("No valid parameters detected!"));
    }
  });
  
  server.on("/rgb", HTTP_GET, [](AsyncWebServerRequest *request){
    if(visorType == "WS2812") {
      visorNow->type++;
      if(visorNow->type == visTypeSize)
        visorNow->type = 0;
      if(cfg.oledEna && oledInitDone)
        oled.writeRGB(vTAcro[visorNow->type]);
        logPrint("[I] Changing visor type to: "+visorTypes[visorNow->type]);
    }
    request->redirect("/saved.html?main");
  });

  server.on("/gyro", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(floor(myIMU.readFloatAccelX()*100)/100)+";"+String(floor(myIMU.readFloatAccelY()*100)/100)+";"+String(floor(myIMU.readFloatAccelZ()*100)/100));
  });

  server.on("/tof", HTTP_GET, [](AsyncWebServerRequest *request){
    if(!ToFInitDone) {
      request->send(200, "text/plain", "ToF not initialized!");
    }
    if (boopMode == "APDS9960") {
        request->send(200, "text/plain", String(255 - apds.readProximity()));
    } else if (boopMode == "VL53L1X") {
      if (vl53.dataReady()) {
        request->send(200, "text/plain", String(vl53.distance()));
      }
      request->send(200, "text/plain", "Data not ready!");
    }
  });
  
  server.on("/log", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", logBuffer);
  });

  server.onNotFound([](AsyncWebServerRequest *request){request->send(404, "text/plain", "Not found");});
  ElegantOTA.begin(&server);
  server.begin();

  ElegantOTA.setAutoReboot(true);
}
