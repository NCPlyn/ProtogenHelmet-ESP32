/*
First HM-10 setup with USB-Serial converter (with proto helmet running)
1. Flash new firmware (v700+)
2. AT (responds: OK)
3. AT+IMME1
4. AT+ROLE1
5. AT+DISC?
6. AT+CONN0 (0 means index of found proto BLE)
Now you can connect the HM-10 to the Arduino
*/

#include <SoftwareSerial.h>
#include <ezButton.h>
#include <EEPROM.h>

#define BTNTIME 1500

ezButton button1(9);
ezButton button2(8);
ezButton button3(7);

SoftwareSerial ble(10, 11); //hm-10: tx rx

bool canSend = false, recon = false;
unsigned long btnPress1 = 900000000000, btnPress2 = 900000000000, btnPress3 = 900000000000, reconTime = 0;
String dataIn;
int btn1Val = 1, btn2Val = 1, btn3Val = 1, numOfAnims = 4;
bool requestBLEsave = false;
unsigned long requestBLEsaveTime = 0;

void setup() {
  Serial.begin(115200);
  ble.begin(115200);
  ble.listen();
  button1.setDebounceTime(50);
  button2.setDebounceTime(50);
  button3.setDebounceTime(50);
  btn1Val = EEPROM.read(0);
  btn2Val = EEPROM.read(1);
  btn3Val = EEPROM.read(2);
  sendD("AT+START");
}

void sendD(String temp) {
  ble.print(temp);
  Serial.print("Sending->");
  Serial.println(temp);
}
void sendI(int temp) {
  ble.print(temp);
  Serial.print("Sending->");
  Serial.println(temp);
}

void changeBtn(int &toChange) {
  if(toChange == numOfAnims) {
    toChange = 1;
  } else {
    toChange++;
  }
  sendI(toChange);
  requestBLEsaveTime = millis();
  requestBLEsave = true;
}

void loop() {
  button1.loop();
  button2.loop();
  button3.loop();
  dataIn = "";
  if(ble.available() > 0){
    dataIn = ble.readStringUntil('\n');
  }
  if (dataIn != "") {
    dataIn.trim();
    Serial.println(dataIn);
    if(dataIn == "OK+CONN") {
      Serial.println("-CONN");
      sendD("AT+NOTIFY_ON000C");
      recon = false;
    } else if(dataIn == "OK+DATA-OK") {
      Serial.println("-DATA-OK");
      sendD("AT+SET_WAYWR000C");
    } else if(dataIn == "OK+SEND-OK") {
      Serial.println("-SEND-OK");
      sendD("g");
      canSend = true;
    } else if(dataIn == "OK+LOST") {
      Serial.println("-LOST");
      canSend = false;
      sendD("AT+CONNL");
      recon = true;
      reconTime = millis();
    } else if(dataIn.charAt(0) == 'i') {
      dataIn.remove(0,1);
      numOfAnims = dataIn.toInt();
      Serial.println(numOfAnims);
    }
  }

  if(recon && !canSend && reconTime+2000 < millis()) {
    sendD("AT+START");
    reconTime = millis();
  }

  if(requestBLEsaveTime+30000 < millis() && requestBLEsave) {
    EEPROM.write(0, btn1Val);
    EEPROM.write(1, btn2Val);
    EEPROM.write(2, btn3Val);
    requestBLEsave = false;
    Serial.println("saving");
  }

  if(button1.isPressed())
    btnPress1 = millis();
  if(button2.isPressed())
    btnPress2 = millis();
  if(button3.isPressed())
    btnPress3 = millis();

  if(button1.isReleased() && canSend) {
    long pressDuration = millis() - btnPress1;
    if(pressDuration < BTNTIME)
      sendI(btn1Val);
      btnPress1 = 900000000000;
  }
  if(button2.isReleased() && canSend) {
    long pressDuration = millis() - btnPress2;
    if(pressDuration < BTNTIME)
      sendI(btn2Val);
      btnPress2 = 900000000000;
  }
  if(button3.isReleased() && canSend) {
    long pressDuration = millis() - btnPress3;
    if(pressDuration < BTNTIME)
      sendI(btn3Val);
      btnPress3 = 900000000000;
  }

  if(btnPress1+1500 < millis() && canSend) {
    btnPress1 = 900000000000;
    changeBtn(btn1Val);
  }
  if(btnPress2+1500 < millis() && canSend) {
    btnPress2 = 900000000000;
    changeBtn(btn2Val);
  }
  if(btnPress3+1500 < millis() && canSend) {
    btnPress3 = 900000000000;
    changeBtn(btn3Val);
  }

  dataIn = "";
}
