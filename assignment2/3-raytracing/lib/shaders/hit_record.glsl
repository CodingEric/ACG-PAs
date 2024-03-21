
#include "material.glsl"

struct HitRecord {
  Material material;
  vec3 normal;
  float t_min;
  float t_max;
};
