#pragma once
#include <cstdint>

#include "pixel.h"

class Image {
 public:
  Image(uint32_t width, uint32_t height);
  explicit Image(const char *image_file_path);
  ~Image();
  Pixel &operator()(int x, int y);
  const Pixel &operator()(int x, int y) const;
  void SaveBMP(const char *save_path);
  void SaveJPG(const char *save_path);
  void SavePNG(const char *save_path);
  [[nodiscard]] uint32_t GetWidth() const;
  [[nodiscard]] uint32_t GetHeight() const;

 private:
  Pixel *pixels_{nullptr};
  uint32_t width_{0};
  uint32_t height_{0};
};
