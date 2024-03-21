#pragma once
#include "glm/glm.hpp"

struct Triangle {
  Triangle(glm::vec3 v0_, glm::vec3 v1_, glm::vec3 v2_)
      : v0(v0_, 0.0f), v1(v1_, 0.0f), v2(v2_, 0.0f) {
  }
  glm::vec4 v0;
  glm::vec4 v1;
  glm::vec4 v2;
};
