#include <glm/glm.hpp>
#include <iostream>

#include "application.h"
#include "draw.h"

int dx[] = {-80, -80, -80, -20, 0, 20,  80,  80,
            80,  80,  80,  20,  0, -20, -80, -80};
int dy[] = {0, 20,  80,  80,  80,  80,  80,  20,
            0, -20, -80, -80, -80, -80, -80, -20};

Pixel HPixel(float H) {
  float R = 0.0f, G = 0.0f, B = 0.0f;
  int S = std::floor(H / 6.0f) * 6.0f;
  H -= S;
  S = H;
  H -= S;
  switch (S) {
    case 0:
      R = 1.0f;
      G = H;
      break;
    case 1:
      R = 1.0f - H;
      G = 1.0f;
      break;
    case 2:
      G = 1.0f;
      B = H;
      break;
    case 3:
      G = 1.0f - H;
      B = 1.0f;
      break;
    case 4:
      B = 1.0f;
      R = H;
      break;
    default:
      B = 1.0f - H;
      R = 1.0f;
      break;
  }
  return Pixel(R, G, B, 1.0f);
}

void Scene() {
  Image img(1200, 900);

  FillRectangle(img, 0, 0, 1279, 299, Pixel(0.2f, 0.2f, 0.2f, 1.0f));
  FillRectangle(img, 0, 300, 1279, 599, Pixel(0.4f, 0.4f, 0.4f, 1.0f));
  FillRectangle(img, 0, 600, 1279, 899, Pixel(0.6f, 0.6f, 0.6f, 1.0f));

  {
    float angle = 0.0f;
    float angles[3] = {0.0f, glm::radians(120.0f), glm::radians(240.0f)};
    glm::vec2 origin{0.0f, 150.0f};
    float H = 0.0f;
    for (int i = 0; i <= 60; i++) {
      angle += glm::radians(4.0f);
      origin += glm::vec2{10.0f, 0.0f};
      glm::vec2 verts[3];
      for (int j = 0; j < 3; j++) {
        glm::vec2 offset{std::sin(angle + angles[j]),
                         std::cos(angle + angles[j])};
        offset *= 60.0f;
        offset += origin;
        verts[j] = offset;
      }
      DrawLine(img, verts[0].x, verts[0].y, verts[1].x, verts[1].y, HPixel(H));
      DrawLine(img, verts[0].x, verts[0].y, verts[2].x, verts[2].y, HPixel(H));
      DrawLine(img, verts[2].x, verts[2].y, verts[1].x, verts[1].y, HPixel(H));
      H += 0.1;
    }
    for (int i = 0; i <= 20; i++) {
      angle += glm::radians(12.0f);
      origin += glm::vec2{30.0f, 0.0f};
      glm::vec2 verts[3];
      for (int j = 0; j < 3; j++) {
        glm::vec2 offset{std::sin(angle + angles[j]),
                         std::cos(angle + angles[j])};
        offset *= 60.0f;
        offset += origin;
        verts[j] = offset;
      }
      FillTriangle(img, verts[0].x + 0.5f, verts[0].y + 0.5f, verts[1].x + 0.5f,
                   verts[1].y + 0.5f, verts[2].x + 0.5f, verts[2].y + 0.5f,
                   HPixel(H));
      H += 0.3;
    }
  }

  {
    float angle = 0.0f;
    float angles[3] = {0.0f, glm::radians(120.0f), glm::radians(240.0f)};
    glm::vec2 origin{0.0f, 450.0f};
    float H = 0.0f;
    for (int i = 0; i <= 2400; i++) {
      angle += glm::radians(0.3f);
      origin += glm::vec2{0.5f, 0.0f};
      glm::vec2 verts[3];
      for (int j = 0; j < 3; j++) {
        glm::vec2 offset{std::sin(angle + angles[j]),
                         std::cos(angle + angles[j])};
        offset *= 60.0f;
        offset += origin;
        verts[j] = offset;
      }
      FillTriangle(img, verts[0].x + 0.5f, verts[0].y + 0.5f, verts[1].x + 0.5f,
                   verts[1].y + 0.5f, verts[2].x + 0.5f, verts[2].y + 0.5f,
                   HPixel(H));
      H += 0.005;
    }
  }

  {
    float angle = 0.0f;
    float angles[3] = {0.0f, glm::radians(120.0f), glm::radians(240.0f)};
    glm::vec2 origin{0.0f, 750.0f};
    float H = 0.0f;
    for (int i = 0; i <= 120; i++) {
      angle += glm::radians(6.0f);
      origin += glm::vec2{10.0f, 0.0f};
      FillCircle(img, origin.x, origin.y, std::cos(angle) * 20.0f + 40.0f,
                 HPixel(H));
      H += 0.1;
    }
  }

  img.SavePNG("scene.png");
  Application app(&img, "Rasterization GUI");
  app.Run();
}

int main() {
  Scene();
}
