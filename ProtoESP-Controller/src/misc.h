#pragma once
#include <math.h>
#include <Arduino.h>

inline bool isApproxEqual(const float ax, const float ay, const float az, const float bx, const float by, const float bz, const float tiltTol)
{
  return (fabs(ax-bx) < tiltTol && fabs(ay-by) < tiltTol && fabs(az-bz) < tiltTol);
}

uint64_t speakMatrix(uint64_t input, uint8_t columns, bool inverted);

inline float mapfloat(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
};

