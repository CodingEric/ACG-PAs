#include <gtest/gtest.h>

#include <fstream>
#include <random>

#include "transformation.h"

TEST(Transformation, Translation) {
  std::mt19937 rd(20221017);
  auto test_func = [&]() {
    std::ifstream file_in(std::string(TEST_DATA_DIR) + "translation.data",
                          std::ios::binary);
    ASSERT_TRUE(file_in.is_open());
    for (int i = 0; i < 10; i++) {
      glm::vec3 v;
      glm::mat4 m;
      file_in.read(reinterpret_cast<char *>(&v), sizeof(v));
      file_in.read(reinterpret_cast<char *>(&m), sizeof(m));
      glm::mat4 ans = Translate(v);

      for (int j = 0; j < 4; j++) {
        for (int k = 0; k < 4; k++) {
          ASSERT_LT(std::abs(ans[j][k] - m[j][k]), 1e-2f);
        }
      }
    }
    file_in.close();
  };
  test_func();
}

TEST(Transformation, Rotation) {
  auto test_func = [&]() {
    std::ifstream file_in(std::string(TEST_DATA_DIR) + "rotation.data",
                          std::ios::binary);
    ASSERT_TRUE(file_in.is_open());
    for (int i = 0; i < 10; i++) {
      glm::vec3 omega;
      float theta;
      glm::mat4 m;
      file_in.read(reinterpret_cast<char *>(&omega), sizeof(omega));
      file_in.read(reinterpret_cast<char *>(&theta), sizeof(theta));
      file_in.read(reinterpret_cast<char *>(&m), sizeof(m));
      glm::mat4 ans = Rotate(omega, theta);

      for (int j = 0; j < 4; j++) {
        for (int k = 0; k < 4; k++) {
          ASSERT_LT(std::abs(ans[j][k] - m[j][k]), 1e-2f);
        }
      }
    }
    file_in.close();
  };
  test_func();
}

TEST(Transformation, LookAt) {
  auto test_func = [&]() {
    std::ifstream file_in(std::string(TEST_DATA_DIR) + "lookat.data",
                          std::ios::binary);
    ASSERT_TRUE(file_in.is_open());
    for (int i = 0; i < 10; i++) {
      glm::vec3 eye;
      glm::vec3 center;
      glm::mat4 m;
      file_in.read(reinterpret_cast<char *>(&eye), sizeof(eye));
      file_in.read(reinterpret_cast<char *>(&center), sizeof(center));
      file_in.read(reinterpret_cast<char *>(&m), sizeof(m));
      glm::mat4 ans = LookAt(eye, center, glm::vec3{0.0f, 1.0f, 0.0f});
      for (int j = 0; j < 4; j++) {
        for (int k = 0; k < 4; k++) {
          ASSERT_LT(std::abs(ans[j][k] - m[j][k]), 1e-2f);
        }
      }
    }
    file_in.close();
  };
  test_func();
}

TEST(Transformation, Perspective) {
  auto test_func = [&]() {
    std::ifstream file_in(std::string(TEST_DATA_DIR) + "perspective.data",
                          std::ios::binary);
    ASSERT_TRUE(file_in.is_open());
    float fovYs[] = {10.0f, 40.0f, 70.0f, 100.0f, 130.0f};
    float aspects[] = {4.0f / 3.0f, 16.0f / 9.0f, 1.0f, 21.0f / 9.0f,
                       9.0f / 16.0f};
    float nearZs[] = {0.1, 1.0f, 0.1, 10.0f, 100.0f};
    float farZs[] = {10.0f, 10000.0f, 0.2f, 1000.0f, 110.0f};
    for (int i = 0; i < 5; i++) {
      glm::mat4 m;
      file_in.read(reinterpret_cast<char *>(&m), sizeof(m));
      glm::mat4 ans =
          Perspective(glm::radians(fovYs[i]), aspects[i], nearZs[i], farZs[i]);

      for (int j = 0; j < 4; j++) {
        for (int k = 0; k < 4; k++) {
          ASSERT_LT(std::abs(ans[j][k] - m[j][k]), 1e-2f);
        }
      }
    }
    file_in.close();
  };
  test_func();
}
