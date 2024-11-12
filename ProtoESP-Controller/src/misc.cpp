#include "misc.h"

bool Misc::isApproxEqual(const float ax, const float ay, const float az, const float bx, const float by, const float bz, const float tiltTol) const {
  return (fabs(ax-bx) < tiltTol && fabs(ay-by) < tiltTol && fabs(az-bz) < tiltTol);
}

float Misc::mapfloat(float x, float in_min, float in_max, float out_min, float out_max) const {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

//--------------------------------//Dynamic speak anim
uint64_t Misc::speakMatrix(uint64_t input) const {
  int darray[8][8];
  for (int i = 0; i < 8; i++) { //convert int64 to 2darray
    uint8_t row = (input >> i * 8) & 0xFF;
    for (int j = 0; j < 8; j++) {
      darray[i][j] = bitRead(row, j);
    }
  }
  for (int i = 0; i < 8; i++) { //do something to 2darray
    for (int j = 0; j < 8; j++) {
      if(darray[j][i] == 1 && j != 0) {
        darray[j-1][i] = 1;
        break;
      }
    }
    for (int j = 7; j > -1; j--) {
      if(darray[j][i] == 1 && j != 7) {
        darray[j+1][i] = 1;
        break;
      }
    }
  }
  uint64_t out = 0;
  for (int i = 0; i < 8; i++) { //convert 2darray to int64
    uint8_t row = 0;
    for (int j = 0; j < 8; j++) {
      bitWrite(row, j, darray[i][j]);
    }
    out |= (uint64_t)row << i * 8;
  }
  return out;
}