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

#include <ezButton.h>
#include <EEPROM.h>

#define BTNTIME 1500
#define BUTTONS 7

ezButton buttonArray[BUTTONS] = {
  ezButton(2), //left
  ezButton(8), //right
  ezButton(3), //up
  ezButton(6), //down
  ezButton(5), //center
  ezButton(7), //up right
  ezButton(4)  //up left
};

unsigned long btnPressTime[BUTTONS];
int btnVal[BUTTONS];

bool canSend = false, recon = false, requestBLEsave = false;
String dataIn;
int numOfAnims = 4;
unsigned long reconTime = 0,requestBLEsaveTime = 0;

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial1.begin(115200);
  //while (!Serial) ;
  for (int i = 0; i < BUTTONS; i++) {
    buttonArray[i].setDebounceTime(50);
    btnVal[i] = EEPROM.read(i);
    btnPressTime[i] = 20000000;
  }
  sendS("AT+START");
}

void sendS(String temp) { //send string
  Serial1.print(temp);
  Serial.print("Sending->");
  Serial.println(temp);
}
void sendI(int temp) { //send integer
  Serial1.print(temp);
  Serial.print("Sending->");
  Serial.println(temp);
}

void changeBtn(int toChange) { //change button value
  if(btnVal[toChange] == numOfAnims) {
    btnVal[toChange] = 1;
  } else {
    btnVal[toChange]++;
  }
  sendI(btnVal[toChange]);
  requestBLEsaveTime = millis();
  requestBLEsave = true;
}

void loop() {
  for (int i = 0; i < BUTTONS; i++) {
    buttonArray[i].loop();
  }
  
  dataIn = "";
  if(Serial1.available() > 0){ //serial read and process 
    dataIn = Serial1.readStringUntil('\n');
  }
  if (dataIn != "") { //try to connect
    dataIn.trim();
    Serial.println(dataIn);
    if(dataIn == "OK+CONN") {
      Serial.println("-CONN");
      sendS("AT+NOTIFY_ON000C");
      recon = false;
    } else if(dataIn == "OK+DATA-OK") {
      Serial.println("-DATA-OK");
      sendS("AT+SET_WAYWR000C");
    } else if(dataIn == "OK+SEND-OK") {
      Serial.println("-SEND-OK");
      sendS("g");
      canSend = true;
    } else if(dataIn == "OK+LOST") {
      Serial.println("-LOST");
      canSend = false;
      sendS("AT+CONNL");
      recon = true;
      reconTime = millis();
    } else if(dataIn.charAt(0) == 'i') {
      dataIn.remove(0,1);
      numOfAnims = dataIn.toInt();
      Serial.println(numOfAnims);
    }
  }

  if(recon && !canSend && reconTime+2000 < millis()) { //reconnect
    sendS("AT+START");
    reconTime = millis();
  }

  if(requestBLEsaveTime+20000 < millis() && requestBLEsave) { //save
    for (int i = 0; i < BUTTONS; i++) {
      EEPROM.write(i, btnVal[i]);
    }
    requestBLEsave = false;
    Serial.println("saved");
  }

  for (int i = 0; i < BUTTONS; i++) {
    if(buttonArray[i].isPressed() && canSend) { //detect press
      btnPressTime[i] = millis();
    }
    if(buttonArray[i].isReleased() && canSend) { //short press
      long pressDuration = millis() - btnPressTime[i];
      if(pressDuration < BTNTIME) {
        sendI(btnVal[i]);
        btnPressTime[i] = 20000000;
      }
    }
    if(btnPressTime[i]+BTNTIME < millis() && canSend) { //long press
      btnPressTime[i] = 20000000;
      changeBtn(i);
    }
  }

  if(!canSend && !buttonArray[3].getState() && !buttonArray[5].getState() && !buttonArray[6].getState()) {
    for (int i = 0; i < BUTTONS; i++) {
      btnVal[i] = 1;
    }
    requestBLEsaveTime = millis();
    requestBLEsave = true;
    Serial.println("reset");
  }

  dataIn = "";
}
