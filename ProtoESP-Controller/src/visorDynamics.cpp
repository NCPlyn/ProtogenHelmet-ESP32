#include "visorDynamics.h"
#include "misc.h"
#include "settings.h"

void dynamicSpeak(uint64_t *leds, bool isMouth[MATRIXESNUM], int volume) {
  int mouthIndexes[MATRIXESNUM];
  int mouthCount = 0;
  for (int i = 0; i < MATRIXESNUM; i++) { // collect true (mouth) indexes
    if (isMouth[i]) {
      mouthIndexes[mouthCount++] = i;
    }
  }
  int half = mouthCount / 2;
  for (int i = 0; i < half; i++) {
    int x = map(volume, 0, 100, 0, half * 8) - (i * 8);
    int leftIndex  = mouthIndexes[half - 1 - i];
    int rightIndex = mouthIndexes[half + i];
    if (x > 0) {
      x = constrain(x, 0, 8);
      leds[leftIndex]  = speakMatrix(leds[leftIndex],  x, true);
      leds[rightIndex] = speakMatrix(leds[rightIndex], x, false);
    }
  }
}

inline void setPixelWS2812(struct CRGB * ledArray, long ledColor, int visorFrame, int y, int i, int j, byte row)
{
  long tempColor = ledColor; //use given color
  if(ledColor == 0) { //if not given a color
    if(visorNow->frames[visorFrame].ppColor[y][(i*8)+j] != 0) { //use ppColor if available
      tempColor = visorNow->frames[visorFrame].ppColor[y][(i*8)+j];
    } else if(visorNow->frames[visorFrame].fColor[y] != 0) { //if not, use fColor if available
      tempColor = visorNow->frames[visorFrame].fColor[y];
    } else { // else config color
      tempColor = cfg.visColor;
    }
  }
  if(oldMatrixFix) {
    ledArray[(y*64)+(i*8)+((i%2!=0)?j:7-j)] = (bitRead(row,j))?tempColor:CRGB::Black; //includes fix for bad rgbmatrix
  } else {
    ledArray[(y*64)+(i*8)+j] = (bitRead(row,j))?tempColor:CRGB::Black;
  }
}

void setMatrixMAX72XX(int y, uint64_t tempLeds[]);
void setMatrixWS2812(struct CRGB * ledArray, long ledColor, int visorFrame, int y, uint64_t tempLeds[]);

void setAllVisor(struct CRGB *ledArray, long ledColor, int visorFrame) {
  uint64_t tempLeds[MATRIXESNUM];
  memcpy(tempLeds, visorNow->frames[visorFrame].leds, sizeof(tempLeds));
  if(speaking) {
    dynamicSpeak(tempLeds, visorNow->isMouth, micVolume);
  }
  if(visorType == "WS2812")
  {
    for(int y = 0; y < numOfSegm; y++) {
      setMatrixWS2812(ledArray, ledColor, visorFrame, y, tempLeds);
    }
  }
  else if(visorType == "MAX72XX")
  {
    for(int y = 0; y < numOfSegm; y++) {
      setMatrixMAX72XX(y, tempLeds);
    }
  }
  FdisplayVisor = true;
}
void setMatrixMAX72XX(int y, uint64_t tempLeds[])
{
  for (int i = 0; i < 8; i++) {
    byte row = (tempLeds[y] >> i * 8) & 0xFF; //---------remove byte from upper global
    for (int j = 0; j < 8; j++) {
        mx.setPoint(i, j+(y*8), bitRead(row, j)); //MAXstuff
    }
  }
}

void setMatrixWS2812(struct CRGB * ledArray, long ledColor, int visorFrame, int y, uint64_t tempLeds[])
{
  for (int i = 0; i < 8; i++) {
    byte row = (tempLeds[y] >> i * 8) & 0xFF; //---------remove byte from upper global
    for (int j = 0; j < 8; j++) {
      setPixelWS2812(ledArray, ledColor, visorFrame, y, i, j, row);
    }
  }
}
