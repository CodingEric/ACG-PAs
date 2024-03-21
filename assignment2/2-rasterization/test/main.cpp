#include <gtest/gtest.h>

#include <fstream>
#include <random>

#include "draw.h"

int dx[] = {-80, -80, -80, -20, 0, 20,  80,  80,
            80,  80,  80,  20,  0, -20, -80, -80};
int dy[] = {0, 20,  80,  80,  80,  80,  80,  20,
            0, -20, -80, -80, -80, -80, -80, -20};

void ValidateResult(Image &img, const char *image_file_path) {
  Image answer(image_file_path);
  for (int i = 0; i < answer.GetWidth(); i++) {
    for (int j = 0; j < answer.GetHeight(); j++) {
      Pixel &ans_color = answer(i, j);
      bool pass = false;
      for (int dx = -1; dx <= 1 && !pass; dx++) {
        for (int dy = -1; dy <= 1 && !pass; dy++) {
          int x = i + dx;
          int y = j + dy;
          if (x < 0 || x >= answer.GetWidth()) {
            continue;
          }
          if (y < 0 || y >= answer.GetHeight()) {
            continue;
          }
          Pixel &img_color = img(x, y);
          if (ans_color.r == img_color.r && ans_color.g == img_color.g &&
              ans_color.b == img_color.b) {
            pass = true;
          }
        }
      }
      ASSERT_TRUE(pass);
    }
  }
}

TEST(Rasterization, Lines) {
  Image img(201, 201);
  for (int i = 0; i < 16; i++) {
    DrawLine(img, 100, 100, 100 + dx[i], 100 + dy[i],
             Pixel(1.0f, 1.0f, 1.0f, 1.0f));
  }
  ValidateResult(img, TEST_DATA_DIR "lines.png");
}

TEST(Rasterization, Triangles) {
  Image img(201, 201);
  for (int i = 0; i < 16; i += 2) {
    int j = (i + 1) & 15;
    FillTriangle(img, 100, 100, 100 + dx[i], 100 + dy[i], 100 + dx[j],
                 100 + dy[j], Pixel(1.0f, 1.0f, 1.0f, 1.0f));
  }
  img.SavePNG(TEST_DATA_DIR "tt.png");
  ValidateResult(img, TEST_DATA_DIR "triangles.png");
}

TEST(Rasterization, Circles) {
  Image img(201, 201);
  FillCircle(img, 100, 100, 80, Pixel(0.5f, 0.5f, 0.5f, 1.0f));
  FillCircle(img, 50, 50, 50, Pixel(1.0f, 0.0f, 1.0f, 1.0f));
  FillCircle(img, 50, 150, 50, Pixel(1.0f, 1.0f, 0.0f, 1.0f));
  FillCircle(img, 150, 50, 50, Pixel(0.0f, 1.0f, 1.0f, 1.0f));
  FillCircle(img, 150, 150, 50, Pixel(1.0f, 1.0f, 1.0f, 1.0f));
  ValidateResult(img, TEST_DATA_DIR "circles.png");
}

TEST(Rasterization, Rectangles) {
  Image img(201, 201);
  std::ifstream file(TEST_DATA_DIR "rectangle.data", std::ios::binary);
  ASSERT_TRUE(file.is_open());
  for (int i = 0; i < 10; i++) {
    int x0, y0, x1, y1;
    Pixel pixel;
    file.read(reinterpret_cast<char *>(&x0), sizeof(x0));
    file.read(reinterpret_cast<char *>(&y0), sizeof(y0));
    file.read(reinterpret_cast<char *>(&x1), sizeof(x1));
    file.read(reinterpret_cast<char *>(&y1), sizeof(y1));
    file.read(reinterpret_cast<char *>(&pixel), sizeof(pixel));
    FillRectangle(img, x0, y0, x1, y1, pixel);
  }
  file.close();
  ValidateResult(img, TEST_DATA_DIR "rectangles.png");
}
