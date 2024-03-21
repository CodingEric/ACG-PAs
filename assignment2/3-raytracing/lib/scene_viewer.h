#pragma once
#include "grassland/grassland.h"
#include "raytracing_lib.h"

using namespace grassland;

class SceneViewer {
 public:
  SceneViewer(Scene *scene,
              const std::string &name,
              int width,
              int height,
              bool headless);
  void Run();

 private:
  void OnUpdate();
  void OnRender();
  void OnClose();
  void OnInit();

  void BuildScreenFrameObjects();

  Scene *scene_{};

  GLFWwindow *window_{};
  std::unique_ptr<vulkan::Core> core_;

  std::unique_ptr<vulkan::ShaderModule> vertex_shader_;
  std::unique_ptr<vulkan::ShaderModule> fragment_shader_;

  std::unique_ptr<vulkan::DescriptorSetLayout> descriptor_set_layout_;
  std::unique_ptr<vulkan::DescriptorPool> descriptor_pool_;
  std::vector<std::unique_ptr<vulkan::DescriptorSet>> descriptor_sets_;

  std::unique_ptr<vulkan::RenderPass> render_pass_;
  std::unique_ptr<vulkan::PipelineLayout> pipeline_layout_;

  std::unique_ptr<vulkan::Pipeline> pipeline_;

  std::unique_ptr<vulkan::Image> framebuffer_image_;
  std::unique_ptr<vulkan::Image> depth_image_;
  std::unique_ptr<vulkan::Framebuffer> framebuffer_;

  std::unique_ptr<vulkan::Buffer> triangle_buffer_;
  std::unique_ptr<vulkan::Buffer> triangle_material_buffer_;
  std::unique_ptr<vulkan::Buffer> sphere_buffer_;
  std::unique_ptr<vulkan::Buffer> sphere_material_buffer_;
  std::unique_ptr<vulkan::Buffer> point_light_buffer_;
  std::vector<std::unique_ptr<vulkan::Buffer>> camera_object_buffers_;
  std::unique_ptr<vulkan::Buffer> camera_object_staging_buffer_;

  int width_;
  int height_;

  bool application_should_close_{};
  std::string name_{};
};
