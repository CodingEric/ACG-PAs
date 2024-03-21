#include "application.h"

#include <iostream>

namespace {
#include "built_in_shaders.inl"
}

Application::Application(Image *image, const std::string &name) : name_(name) {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  window_ = glfwCreateWindow(image->GetWidth(), image->GetHeight(),
                             name.c_str(), nullptr, nullptr);

  vulkan::CoreSettings core_settings;
  core_settings.window = window_;
  core_ = std::make_unique<vulkan::Core>(core_settings);

  glfwSetWindowUserPointer(window_, this);

  image_ = std::make_unique<vulkan::Image>(
      core_.get(), VK_FORMAT_R8G8B8A8_UNORM,
      VkExtent2D{image->GetWidth(), image->GetHeight()},
      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

  vulkan::Buffer staging_buffer(
      core_.get(), image->GetWidth() * image->GetHeight() * sizeof(Pixel),
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

  void *data = staging_buffer.Map();
  std::memcpy(data, &image->operator()(0, 0),
              image->GetWidth() * image->GetHeight() * sizeof(Pixel));
  staging_buffer.Unmap();

  core_->SingleTimeCommands([&](VkCommandBuffer command_buffer) {
    // Transit from undefined to transfer destination
    vulkan::TransitImageLayout(
        command_buffer, image_->Handle(), VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_NONE,
        VK_ACCESS_TRANSFER_WRITE_BIT);

    // Copy from staging buffer to image
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent.width = image->GetWidth();
    region.imageExtent.height = image->GetHeight();
    region.imageExtent.depth = 1;
    vkCmdCopyBufferToImage(command_buffer, staging_buffer.Handle(),
                           image_->Handle(),
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // Transit from transfer destination to shader read only
    vulkan::TransitImageLayout(
        command_buffer, image_->Handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);
  });
}

void Application::Run() {
  OnInit();
  if (window_) {
    while (!glfwWindowShouldClose(window_)) {
      glfwPollEvents();
      OnUpdate();
      OnRender();
    }
  } else {
    while (!application_should_close_) {
      OnUpdate();
      OnRender();
    }
  }
  OnClose();
}

void Application::OnUpdate() {
}

void Application::OnRender() {
  core_->BeginFrame();
  auto command_buffer = core_->CommandBuffer();

  // Begin render pass
  VkClearValue clear_value{};
  clear_value.color = {0.0f, 0.0f, 0.0f, 1.0f};
  VkRenderPassBeginInfo render_pass_begin_info{};
  render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_begin_info.renderPass = render_pass_->Handle();
  render_pass_begin_info.framebuffer = framebuffer_->Handle();
  render_pass_begin_info.renderArea.extent = core_->SwapChain()->Extent();
  render_pass_begin_info.clearValueCount = 1;
  render_pass_begin_info.pClearValues = &clear_value;
  vkCmdBeginRenderPass(command_buffer->Handle(), &render_pass_begin_info,
                       VK_SUBPASS_CONTENTS_INLINE);

  // Bind pipeline
  vkCmdBindPipeline(command_buffer->Handle(), VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipeline_->Handle());

  // Bind descriptor set
  VkDescriptorSet descriptor_set = descriptor_set_->Handle();
  vkCmdBindDescriptorSets(
      command_buffer->Handle(), VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline_layout_->Handle(), 0, 1, &descriptor_set, 0, nullptr);

  // Set scissor and viewport
  VkViewport viewport{};
  viewport.width = core_->SwapChain()->Extent().width;
  viewport.height = core_->SwapChain()->Extent().height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(command_buffer->Handle(), 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.extent = core_->SwapChain()->Extent();
  vkCmdSetScissor(command_buffer->Handle(), 0, 1, &scissor);

  // Draw indexed
  vkCmdBindIndexBuffer(command_buffer->Handle(), index_buffer_->Handle(), 0,
                       VK_INDEX_TYPE_UINT16);

  vkCmdDrawIndexed(command_buffer->Handle(), 6, 1, 0, 0, 0);

  // End render pass
  vkCmdEndRenderPass(command_buffer->Handle());

  // Transit framebuffer image layout
  vulkan::TransitImageLayout(
      command_buffer->Handle(),
      core_->SwapChain()->Images()[core_->ImageIndex()],
      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_ACCESS_NONE, VK_ACCESS_TRANSFER_WRITE_BIT);

  // Copy framebuffer image to swap chain image
  VkImageCopy image_copy{};
  image_copy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  image_copy.srcSubresource.layerCount = 1;
  image_copy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  image_copy.dstSubresource.layerCount = 1;
  image_copy.extent.width = core_->SwapChain()->Extent().width;
  image_copy.extent.height = core_->SwapChain()->Extent().height;
  image_copy.extent.depth = 1;
  vkCmdCopyImage(command_buffer->Handle(), frame_image_->Handle(),
                 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                 core_->SwapChain()->Images()[core_->ImageIndex()],
                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &image_copy);

  // Transit framebuffer image layout
  vulkan::TransitImageLayout(
      command_buffer->Handle(),
      core_->SwapChain()->Images()[core_->ImageIndex()],
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
      VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
      VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_NONE);

  core_->EndFrame();
}

void Application::OnInit() {
  frame_image_ = std::make_unique<vulkan::Image>(
      core_.get(), core_->SwapChain()->Format(), core_->SwapChain()->Extent(),
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
      VK_IMAGE_ASPECT_COLOR_BIT, VK_SAMPLE_COUNT_1_BIT);

  sampler_ = std::make_unique<vulkan::Sampler>(
      core_.get(), VK_FILTER_LINEAR, VK_FILTER_LINEAR,
      VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT,
      VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_FALSE,
      VK_BORDER_COLOR_INT_OPAQUE_BLACK, VK_SAMPLER_MIPMAP_MODE_LINEAR);

  // Define attachment descriptions and color references
  std::vector<VkAttachmentDescription> attachment_descriptions{
      VkAttachmentDescription{
          0, core_->SwapChain()->Format(), VK_SAMPLE_COUNT_1_BIT,
          VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
          VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
          VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL}};

  std::vector<VkAttachmentReference> color_references{
      VkAttachmentReference{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}};

  render_pass_ = std::make_unique<vulkan::RenderPass>(
      core_.get(), attachment_descriptions, color_references);
  framebuffer_ = std::make_unique<vulkan::Framebuffer>(
      core_.get(), core_->SwapChain()->Extent(), render_pass_->Handle(),
      std::vector<VkImageView>{frame_image_->ImageView()});

  std::vector<uint16_t> indices{0, 1, 2, 2, 3, 1};

  index_buffer_ = std::make_unique<vulkan::Buffer>(
      core_.get(), indices.size() * sizeof(uint16_t),
      VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VMA_MEMORY_USAGE_GPU_ONLY);

  vulkan::UploadBuffer(index_buffer_.get(), indices.data(),
                       indices.size() * sizeof(uint16_t));

  descriptor_pool_ = std::make_unique<vulkan::DescriptorPool>(
      core_.get(),
      std::vector<VkDescriptorPoolSize>{
          VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}},
      1);
  descriptor_set_layout_ = std::make_unique<vulkan::DescriptorSetLayout>(
      core_.get(),
      std::vector<VkDescriptorSetLayoutBinding>{VkDescriptorSetLayoutBinding{
          0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
          VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}});
  descriptor_set_ = std::make_unique<vulkan::DescriptorSet>(
      core_.get(), descriptor_pool_.get(), descriptor_set_layout_.get());

  VkDescriptorImageInfo image_info{};
  image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  image_info.imageView = image_->ImageView();
  image_info.sampler = sampler_->Handle();

  VkWriteDescriptorSet write_descriptor_set{};
  write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write_descriptor_set.dstSet = descriptor_set_->Handle();
  write_descriptor_set.dstBinding = 0;
  write_descriptor_set.dstArrayElement = 0;
  write_descriptor_set.descriptorType =
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  write_descriptor_set.descriptorCount = 1;
  write_descriptor_set.pImageInfo = &image_info;

  vkUpdateDescriptorSets(core_->Device()->Handle(), 1, &write_descriptor_set, 0,
                         nullptr);

  vertex_shader_ = std::make_unique<vulkan::ShaderModule>(
      core_.get(),
      vulkan::CompileGLSLToSPIRV(GetShaderCode("shader/output_texture.vert"),
                                 VK_SHADER_STAGE_VERTEX_BIT));
  fragment_shader_ = std::make_unique<vulkan::ShaderModule>(
      core_.get(),
      vulkan::CompileGLSLToSPIRV(GetShaderCode("shader/output_texture.frag"),
                                 VK_SHADER_STAGE_FRAGMENT_BIT));

  pipeline_layout_ = std::make_unique<vulkan::PipelineLayout>(
      core_.get(),
      std::vector<vulkan::DescriptorSetLayout *>{descriptor_set_layout_.get()});

  vulkan::PipelineSettings pipeline_settings(render_pass_.get(),
                                             pipeline_layout_.get());
  pipeline_settings.AddShaderStage(vertex_shader_.get(),
                                   VK_SHADER_STAGE_VERTEX_BIT);
  pipeline_settings.AddShaderStage(fragment_shader_.get(),
                                   VK_SHADER_STAGE_FRAGMENT_BIT);

  pipeline_ =
      std::make_unique<vulkan::Pipeline>(core_.get(), pipeline_settings);
}

void Application::OnClose() {
  // Release resources in reverse order of creation
  core_->Device()->WaitIdle();
  pipeline_.reset();
  pipeline_layout_.reset();
  fragment_shader_.reset();
  vertex_shader_.reset();
  descriptor_set_.reset();
  descriptor_set_layout_.reset();
  descriptor_pool_.reset();
  index_buffer_.reset();
  framebuffer_.reset();
  render_pass_.reset();
  frame_image_.reset();
  sampler_.reset();
  image_.reset();
  core_.reset();
  if (window_) {
    glfwDestroyWindow(window_);
    glfwTerminate();
  }
}
