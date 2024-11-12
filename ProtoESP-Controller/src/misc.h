#include <math.h>
#include <Arduino.h>

class Misc {
public:
  bool isApproxEqual(const float ax, const float ay, const float az, const float bx, const float by, const float bz, const float tiltTol) const;
  uint64_t speakMatrix(uint64_t input) const;
  float mapfloat(float x, float in_min, float in_max, float out_min, float out_max) const;
};