#include "transformation.h"

#include <cmath>
#include <glm/glm.hpp>

glm::mat4 Translate(const glm::vec3 &v) {
  /*
   * TODO: Implement a function that returns translation matrix
   * @v: translation vector
   * */
  return glm::mat4{1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                   0.0f, 0.0f, 1.0f, 0.0f, v[0], v[1], v[2], 1.0f};
}

glm::mat4 Rotate(const glm::vec3 &omega, float theta) {
  /*
   * TODO: Implement a function that returns rotation matrix
   * We guarantee that:
   * @omega: the rotation axis, |omega| = 1
   * @theta: the rotation angle (in radians)
   * */
  glm::mat3 ssm = glm::mat3({0.0f, omega[2], -omega[1], -omega[2], 0.0f,
                             omega[0], omega[1], -omega[0], 0.0f});
  return glm::mat4(glm::mat3(1.0f) + std::sin(theta) * ssm +
                   (1 - std::cos(theta)) * ssm * ssm);
}

glm::mat4 LookAt(const glm::vec3 &eye,
                 const glm::vec3 &center,
                 const glm::vec3 &up) {
  /*
   * TODO: Implement a function that returns world transform matrix
   * We guarantee that:
   * length(cross(center - eye, up)) > 1e-3
   * @eye: the position of the camera
   * @center: the position where the camera is looking at
   * @up: the up vector of the camera
   * */
  glm::vec3 negz = glm::normalize(center - eye);
  glm::vec3 x = glm::normalize(glm::cross(center - eye, up));
  glm::vec3 y = glm::cross(x, negz);
  glm::mat3 c2w(glm::mat3(x, y, -negz));
  glm::mat4 ret(glm::transpose(c2w));
  ret[3] = glm::vec4(glm::dot(-eye, x), glm::dot(-eye, y), glm::dot(eye, negz),
                     1.0f);
  return ret;
}

glm::mat4 Perspective(float fovY, float aspect, float nearZ, float farZ) {
  /*
   * TODO: Implement a function that generate perspective projection matrix.
   * @fovY: field of view angle (in radians) in the y direction
   * @aspect: aspect ratio of x (width) to y (height)
   * @nearZ: distance from the viewer to the near clipping plane (only positive)
   * @farZ: distance from the viewer to the far clipping plane (only positive)
   * */
  glm::mat4 ret(0.0f);

  float f = 1.0f / tan(fovY / 2.0f);
  float rangeInv = 1.0f / (nearZ - farZ);

  ret[0][0] = f / aspect;
  ret[1][1] = f;
  ret[2][2] = (nearZ + farZ) * rangeInv;
  ret[2][3] = -1.0f;
  ret[3][2] = 2.0f * nearZ * farZ * rangeInv;

  return ret;
}
