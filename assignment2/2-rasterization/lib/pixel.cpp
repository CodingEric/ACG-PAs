#include "pixel.h"

#include <algorithm>

namespace {
uint8_t FloatTo255(float c) {
  c = std::min(1.0f, std::max(c, 0.0f));
  return int(c * 255) & 0xff;
}
}  // namespace

Pixel::Pixel(float r, float g, float b, float a)
    : Pixel(FloatTo255(r), FloatTo255(g), FloatTo255(b), FloatTo255(a)) {
}

Pixel::Pixel(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
  this->r = r;
  this->g = g;
  this->b = b;
  this->a = a;
}

Pixel::Pixel(int r, int g, int b, int a) {
  this->r = r;
  this->g = g;
  this->b = b;
  this->a = a;
}

uint8_t Pixel::Gray() const {
  return uint8_t((r * 19595 + g * 38469 + b * 7472) >> 16);
}

Pixel::Pixel() = default;
