#include "image.h"

#include <cassert>
#include <cstring>
#include <memory>
#include <string>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

Image::Image(uint32_t width, uint32_t height) {
  width_ = width;
  height_ = height;
  pixels_ = new Pixel[width * height];
  assert(pixels_);
}

Image::~Image() {
  delete[] pixels_;
}

Pixel &Image::operator()(int x, int y) {
  assert(x >= 0);
  assert(x < width_);
  assert(y >= 0);
  assert(y < height_);
  return pixels_[y * width_ + x];
}

const Pixel &Image::operator()(int x, int y) const {
  assert(x >= 0);
  assert(x < width_);
  assert(y >= 0);
  assert(y < height_);
  return pixels_[y * width_ + x];
}

Image::Image(const char *image_file_path) {
  int comp;
  auto data = stbi_load(image_file_path, reinterpret_cast<int *>(&width_),
                        reinterpret_cast<int *>(&height_), &comp, 4);
  pixels_ = new Pixel[width_ * height_];
  assert(pixels_);
  std::memcpy(pixels_, data, width_ * height_ * sizeof(Pixel));
  stbi_image_free(data);
}

void Image::SaveBMP(const char *save_path) {
  stbi_write_bmp(save_path, width_, height_, 4, pixels_);
}

void Image::SaveJPG(const char *save_path) {
  stbi_write_jpg(save_path, width_, height_, 4, pixels_, 100);
}

void Image::SavePNG(const char *save_path) {
  stbi_write_png(save_path, width_, height_, 4, pixels_,
                 sizeof(Pixel) * width_);
}

uint32_t Image::GetWidth() const {
  return width_;
}

uint32_t Image::GetHeight() const {
  return height_;
}
