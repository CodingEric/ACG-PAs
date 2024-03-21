#include <iostream>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "gtest/gtest.h"
#include "raytracing_lib.h"
#include "scene_viewer.h"
#include "stb_image.h"

TEST(RayTracing, TestScene) {
  SceneSettings scene_settings;
  Scene scene;
  scene.AddSphere({{0.0f, 0.0f, 0.0f}, 1e3f},
                  {{0.6f, 0.7f, 0.8f}, kLambertian});

  float reflect_index = 0.8f;
  scene.AddSphere({{0.0f, 0.0f, 0.0f}, 3000.0f},
                  {glm::vec3{0.6f, 0.7f, 0.8f}, kLambertian});
  scene.AddSphere({{0.0f, 2.0f, 0.0f}, 2.0f},
                  {{0.2001f, 0.5f, 0.18f}, kLambertian});
  scene.AddSphere({{2, 1.6, 3}, 1.6f},
                  {glm::vec3{1.0, 0.5, 1.0} * reflect_index, kLambertian});
  scene.AddSphere({{-4, 1.0, 3}, 1.0f},
                  {glm::vec3{1.0, 1.0, 1.0} * reflect_index, kSpecular});
  scene.AddSphere({{3.5, 1.4, -2.5}, 1.4f},
                  {glm::vec3{1.0, 1.0, 1.0} * reflect_index, kSpecular});
  scene.AddSphere({{-1.5, 1.2, -4.5}, 1.2f},
                  {glm::vec3{0.5, 1.0, 1.0} * reflect_index, kLambertian});
  scene.AddSphere({{-5.5, 3.0, -1.5}, 3.0f},
                  {glm::vec3{1.0, 1.0, 1.0} * reflect_index, kSpecular});
  scene.AddSphere({{-3.5, 2.5, -12.5}, 2.5f},
                  {glm::vec3{1.0, 1.0, 0.5} * reflect_index, kLambertian});

  scene.AddTriangle(
      {{-1e2f, 0.0f, -1e2f}, {1e2f, 0.0f, -1e2f}, {-1e2f, 0.0f, 1e2f}},
      {{0.8f, 0.8f, 0.8f}, kLambertian});
  scene.AddTriangle(
      {{1e2f, 0.0f, 1e2f}, {1e2f, 0.0f, -1e2f}, {-1e2f, 0.0f, 1e2f}},
      {{0.8f, 0.8f, 0.8f}, kLambertian});
  scene.AddPointLight(
      {glm::normalize(glm::vec3{0.0f, 6.0f, 15.0f}) * 900.0f, 7e5f});

  scene_settings.ambient_color = glm::vec3{0.3f};
  scene_settings.camera_position = glm::vec3{10.0f, 5.0f, 10.0f};
  scene_settings.look_at = glm::vec3{0.0f, 1.0f, 0.0f};
  scene_settings.fov_y = glm::radians(40.0f);
  scene.SetSceneSettings(scene_settings);
  SceneViewer scene_viewer(&scene, "Ray Tracing Test", 1280, 720, true);
  scene_viewer.Run();

  int answer_x, answer_y, answer_c;
  int result_x, result_y, result_c;
  auto answer_image = stbi_load(TEST_DATA_DIR "raytracing.png", &answer_x,
                                &answer_y, &answer_c, 4);
  auto result_image =
      stbi_load("raytracing_result.png", &result_x, &result_y, &result_c, 4);
  ASSERT_TRUE(answer_image);
  ASSERT_TRUE(result_image);
  ASSERT_EQ(answer_x, result_x);
  ASSERT_EQ(answer_y, result_y);
  int match_count = 0;
  for (int y = 0; y < answer_y; y++) {
    for (int x = 0; x < answer_x; x++) {
      bool match = true;
      int index = y * answer_x + x;
      for (int c = 0; c < 3; c++) {
        if (std::abs(int(answer_image[(index << 2) + c]) -
                     int(result_image[(index << 2) + c])) > 4) {
          match = false;
          break;
        }
      }
      if (match) {
        match_count++;
      }
    }
  }

  stbi_image_free(answer_image);
  stbi_image_free(result_image);
  double match_rate = double(match_count) / double(answer_x * answer_y);
  EXPECT_GT(match_rate, 0.99);
}
