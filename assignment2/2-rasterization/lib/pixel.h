#pragma once
#include <cstdint>

struct Pixel {
  uint8_t r{};
  uint8_t g{};
  uint8_t b{};
  uint8_t a{};
  Pixel();
  Pixel(float r, float g, float b, float a = 1.0f);
  Pixel(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0xffu);
  Pixel(int r, int g, int b, int a = 0xffu);
  [[nodiscard]] uint8_t Gray() const;
};
