#pragma once
#include "grassland/grassland.h"
#include "image.h"
#include "map"

using namespace grassland;

class Application {
 public:
  Application(Image *image, const std::string &name);
  void Run();

 private:
  void OnUpdate();
  void OnRender();
  void OnClose();
  void OnInit();

  GLFWwindow *window_{};
  std::unique_ptr<vulkan::Core> core_;
  std::unique_ptr<vulkan::Image> image_;
  std::unique_ptr<vulkan::Sampler> sampler_;
  std::unique_ptr<vulkan::Image> frame_image_;
  std::unique_ptr<vulkan::RenderPass> render_pass_;
  std::unique_ptr<vulkan::Framebuffer> framebuffer_;

  std::unique_ptr<vulkan::Buffer> index_buffer_;

  // Descriptor pool and set and set layout
  std::unique_ptr<vulkan::DescriptorPool> descriptor_pool_;
  std::unique_ptr<vulkan::DescriptorSetLayout> descriptor_set_layout_;
  std::unique_ptr<vulkan::DescriptorSet> descriptor_set_;

  std::unique_ptr<vulkan::ShaderModule> vertex_shader_;
  std::unique_ptr<vulkan::ShaderModule> fragment_shader_;

  // Pipeline layout
  std::unique_ptr<vulkan::PipelineLayout> pipeline_layout_;
  std::unique_ptr<vulkan::Pipeline> pipeline_;

  bool application_should_close_{};
  std::string name_{};
};
