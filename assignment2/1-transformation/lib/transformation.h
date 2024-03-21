#pragma once
#include <glm/glm.hpp>

glm::mat4 Translate(const glm::vec3 &v);

glm::mat4 Rotate(const glm::vec3 &omega, float theta);

glm::mat4 LookAt(const glm::vec3 &eye,
                 const glm::vec3 &center,
                 const glm::vec3 &up);

glm::mat4 Perspective(float fovY, float aspect, float nearZ, float farZ);
