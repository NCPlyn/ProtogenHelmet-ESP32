#include "misc.h"
#include <stdint.h>

//--------------------------------//Dynamic speak anim
uint64_t speakMatrix(uint64_t input, uint8_t columns, bool inverted)
{
  uint8_t rows[8];

  // convert uint64_t to uint8_t rows
  for(int i = 0; i < 8; i++)
    rows[i] = (input >> (i * 8)) & 0xFF;

  // clamp columns to valid range
  if(columns > 8) columns = 8;

  // process only selected columns
  for(uint8_t c = 0; c < columns; c++)
  {
    uint8_t i = inverted ? (7 - c) : c;

    for(int8_t j = 1; j < 8; j++)
    {
      if(bitRead(rows[j], i) != 1) continue;
      bitWrite(rows[j - 1], i, 1);
      break;
    }

    for(int8_t j = 6 /* 7 - 1 */ ; j >= 0; j--)
    {
      if(bitRead(rows[j], i) != 1) continue;
      bitWrite(rows[j + 1], i, 1);
      break;
    }
  }
  // convert uint8_t rows back to uint64_t
  uint64_t out = 0;
  for(int i = 0; i < 8; i++)
    out |= (uint64_t)rows[i] << (i * 8);
  return out;
}

