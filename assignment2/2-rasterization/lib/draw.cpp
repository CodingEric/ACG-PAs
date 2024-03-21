#include "draw.h"

#include <algorithm>
#include <cmath>

void DrawPixel(Image &img, int x, int y, const Pixel &color) {
}

bool isPointValid(int x, int y, const Image &img) {
  if (x < 0 || x >= img.GetWidth()) {
    return false;
  }
  if (y < 0 || y >= img.GetHeight()) {
    return false;
  }
  return true;
}

void DrawLine(Image &img, int x0, int y0, int x1, int y1, const Pixel &color) {
  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = dx + dy, e2;

  for (;;) {
    if (isPointValid(x0, y0, img))
      img(x0, y0) = color;
    if (x0 == x1 && y0 == y1)
      break;
    e2 = 2 * err;
    if (e2 >= dy) {
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx) {
      err += dx;
      y0 += sy;
    }
  }
}

void FillRectangle(Image &img,
                   int x0,
                   int y0,
                   int x1,
                   int y1,
                   const Pixel &color) {
  int xmin = std::max(0, std::min(x0, x1));
  int ymin = std::max(0, std::min(y0, y1));
  int xmax = std::min(std::max(x0, x1), (int)img.GetWidth() - 1);
  int ymax = std::min(std::max(y0, y1), (int)img.GetHeight() - 1);
  for (int x = xmin; x <= xmax; ++x) {
    for (int y = ymin; y <= ymax; ++y) {
      img(x, y) = color;
    }
  }
}

void FillCircle(Image &img, int x, int y, int r, const Pixel &color) {
  int xmin = x - r;
  int ymin = y - r;
  int xmax = x + r;
  int ymax = y + r;
  int r2 = r * r;
  xmin = std::max(0, xmin);
  ymin = std::max(0, ymin);
  xmax = std::min((int)img.GetWidth() - 1, xmax);
  ymax = std::min((int)img.GetHeight() - 1, ymax);
  for (int xx = xmin; xx <= xmax; ++xx) {
    for (int yy = ymin; yy <= ymax; ++yy) {
      if ((xx - x) * (xx - x) + (yy - y) * (yy - y) <= r2)
        img(xx, yy) = color;
    }
  }
}

void fillBottomFlatTriangle(int x0,
                            int x1,
                            int x2,
                            int y0,
                            int y1,
                            int y2,
                            Image &img,
                            const Pixel &color) {
  float invslope1 = float(x1 - x0) / float(y1 - y0);
  float invslope2 = float(x2 - x0) / float(y2 - y0);

  float curx1 = x0;
  float curx2 = x0;

  for (int scanlineY = y0; scanlineY <= y1; scanlineY++) {
    for (int i = std::min(std::round(curx2), std::round(curx1));
         i <= std::max((int)curx2, (int)curx1); ++i) {
      if (isPointValid(i, scanlineY, img))
        img(i, scanlineY) = color;
    }
    curx1 += invslope1;
    curx2 += invslope2;
  }
}

void fillTopFlatTriangle(int x0,
                         int x1,
                         int x2,
                         int y0,
                         int y1,
                         int y2,
                         Image &img,
                         const Pixel &color) {
  float invslope1 = float(x2 - x0) / float(y2 - y0);
  float invslope2 = float(x2 - x1) / float(y2 - y1);

  float curx1 = x2;
  float curx2 = x2;

  for (int scanlineY = y2; scanlineY >= y0; scanlineY--) {
    for (int i = std::min(std::round(curx2), std::round(curx1));
         i <= std::max((int)curx2, (int)curx1); ++i) {
      if (isPointValid(i, scanlineY, img))
        img(i, scanlineY) = color;
    }
    curx1 -= invslope1;
    curx2 -= invslope2;
  }
}

void FillTriangle(Image &img,
                  int x0,
                  int y0,
                  int x1,
                  int y1,
                  int x2,
                  int y2,
                  const Pixel &color) {
  if (y0 > y1) {
    std::swap(x0, x1);
    std::swap(y0, y1);
  }
  if (y1 > y2) {
    std::swap(x1, x2);
    std::swap(y1, y2);
  }
  if (y0 > y2) {
    std::swap(x0, x2);
    std::swap(y0, y2);
  }
  if (y0 > y1) {
    std::swap(x0, x1);
    std::swap(y0, y1);
  }

  if (y1 == y2) {
    fillBottomFlatTriangle(x0, x1, x2, y0, y1, y2, img, color);
  } else if (y0 == y1) {
    fillTopFlatTriangle(x0, x1, x2, y0, y1, y2, img, color);
  } else {
    int x3 = (int)(x0 + ((float)(y1 - y0) / (float)(y2 - y0)) * (x2 - x0));
    int y3 = y1;
    fillBottomFlatTriangle(x0, x1, x3, y0, y1, y3, img, color);
    fillTopFlatTriangle(x1, x3, x2, y1, y3, y2, img, color);
  }
}
