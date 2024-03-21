#pragma once
#include "image.h"

void DrawPixel(Image &img, int x, int y, const Pixel &color);

void DrawLine(Image &img, int x0, int y0, int x1, int y1, const Pixel &color);

void FillRectangle(Image &img,
                   int x0,
                   int y0,
                   int x1,
                   int y1,
                   const Pixel &color);

void FillCircle(Image &img, int x, int y, int r, const Pixel &color);

void FillTriangle(Image &img,
                  int x0,
                  int y0,
                  int x1,
                  int y1,
                  int x2,
                  int y2,
                  const Pixel &color);
