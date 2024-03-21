#include "scene_viewer.h"

#include <iostream>

#include "glm/gtc/matrix_transform.hpp"
#include "stb_image_write.h"

namespace {
#include "built_in_shaders.inl"
}

SceneViewer::SceneViewer(Scene *scene,
                         const std::string &name,
                         int width,
                         int height,
                         bool headless)
    : name_(name),
      scene_(scene),
      width_(width),
      height_(height),
      application_should_close_(false) {
  if (!headless) {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window_ = glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr);
  }

  vulkan::CoreSettings core_settings;
  core_settings.window = window_;
  core_ = std::make_unique<vulkan::Core>(core_settings);

  if (!headless) {
    glfwSetWindowUserPointer(window_, this);
    glfwSetFramebufferSizeCallback(
        window_, [](GLFWwindow *window, int width, int height) {
          auto app =
              reinterpret_cast<SceneViewer *>(glfwGetWindowUserPointer(window));
          app->core_->Device()->WaitIdle();
          app->core_->RebuildSwapChain();
          app->BuildScreenFrameObjects();
        });
  }
}

void SceneViewer::Run() {
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
      application_should_close_ = true;
    }
    core_->Device()->WaitIdle();
    // Download data from framebuffer_image
    std::vector<uint32_t> framebuffer_data(framebuffer_image_->Extent().width *
                                           framebuffer_image_->Extent().height);
    auto staging_buffer = std::make_unique<vulkan::Buffer>(
        core_.get(), framebuffer_data.size() * sizeof(uint32_t),
        VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

    core_->SingleTimeCommands([&](VkCommandBuffer cmd_buffer) {
      // Copy from image to buffer
      VkBufferImageCopy buffer_image_copy{};
      buffer_image_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      buffer_image_copy.imageSubresource.layerCount = 1;
      buffer_image_copy.imageExtent.width = framebuffer_image_->Extent().width;
      buffer_image_copy.imageExtent.height =
          framebuffer_image_->Extent().height;
      buffer_image_copy.imageExtent.depth = 1;
      vkCmdCopyImageToBuffer(cmd_buffer, framebuffer_image_->Handle(),
                             VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                             staging_buffer->Handle(), 1, &buffer_image_copy);
    });

    core_->Device()->SingleTimeCommandQueue().WaitIdle();

    // Map to host buffer first, then copy to device buffer
    void *data = staging_buffer->Map();
    memcpy(framebuffer_data.data(), data,
           framebuffer_data.size() * sizeof(uint32_t));
    staging_buffer->Unmap();

    // Save to file
    stbi_write_png("raytracing_result.png", framebuffer_image_->Extent().width,
                   framebuffer_image_->Extent().height, 4,
                   framebuffer_data.data(),
                   framebuffer_image_->Extent().width * sizeof(uint32_t));
  }

  OnClose();
}

void SceneViewer::OnUpdate() {
  auto looking_direction =
      glm::normalize(scene_->GetSceneSettings().look_at -
                     scene_->GetSceneSettings().camera_position);
  static float pitch = std::asin(looking_direction.y),
               yaw = std::atan2(looking_direction.x, looking_direction.z);

  CameraObject camera_object{};

  camera_object.window_extent = {framebuffer_image_->Extent().width,
                                 framebuffer_image_->Extent().height};
  camera_object.projection = glm::inverse(
      glm::mat4{1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f} *
      glm::perspectiveLH_ZO(scene_->GetSceneSettings().fov_y,
                            float(framebuffer_image_->Extent().width) /
                                float(framebuffer_image_->Extent().height),
                            0.1f, 1.0f));
  auto camera_to_world = glm::inverse(
      glm::rotate(glm::mat4{1.0f}, pitch, glm::vec3{1.0f, 0.0f, 0.0f}) *
      glm::rotate(glm::mat4{1.0f}, yaw, glm::vec3{0.0f, -1.0f, 0.0f}) *
      glm::translate(glm::mat4{1.0f},
                     -scene_->GetSceneSettings().camera_position));
  camera_object.camera_to_world = camera_to_world;
  camera_object.num_triangle = scene_->GetTriangleBuffer().size();
  camera_object.num_sphere = scene_->GetSphereBuffer().size();
  camera_object.ambient_light = scene_->GetSceneSettings().ambient_color;
  camera_object.num_point_light = scene_->GetPointLightBuffer().size();

  // Map to host buffer first, then copy to device buffer
  void *data = camera_object_staging_buffer_->Map();
  memcpy(data, &camera_object, sizeof(camera_object));
  camera_object_staging_buffer_->Unmap();
  core_->SingleTimeCommands([&](VkCommandBuffer cmd_buffer) {
    vulkan::CopyBuffer(cmd_buffer, camera_object_staging_buffer_.get(),
                       camera_object_buffers_[core_->CurrentFrame()].get(),
                       sizeof(CameraObject));
  });

  // Calculate time duration of last frame using static timestamp
  static auto last_frame_time = std::chrono::high_resolution_clock::now();
  auto current_frame_time = std::chrono::high_resolution_clock::now();
  float delta_time = std::chrono::duration<float, std::chrono::seconds::period>(
                         current_frame_time - last_frame_time)
                         .count();
  last_frame_time = current_frame_time;

  if (window_) {
    auto scene_settings = scene_->GetSceneSettings();
    glm::vec3 move{0.0f};
    float speed = 30.0f;
    if (glfwGetKey(window_, GLFW_KEY_LEFT_SHIFT)) {
      speed /= 10.0f;
    }
    if (glfwGetKey(window_, GLFW_KEY_W)) {
      move.z += delta_time * speed;
    }
    if (glfwGetKey(window_, GLFW_KEY_S)) {
      move.z -= delta_time * speed;
    }
    if (glfwGetKey(window_, GLFW_KEY_A)) {
      move.x -= delta_time * speed;
    }
    if (glfwGetKey(window_, GLFW_KEY_D)) {
      move.x += delta_time * speed;
    }
    if (glfwGetKey(window_, GLFW_KEY_SPACE)) {
      move.y += delta_time * speed;
    }
    if (glfwGetKey(window_, GLFW_KEY_LEFT_CONTROL)) {
      move.y -= delta_time * speed;
    }

    double current_cursor_x, current_cursor_y;
    glfwGetCursorPos(window_, &current_cursor_x, &current_cursor_y);
    static double last_cursor_x = current_cursor_x,
                  last_cursor_y = current_cursor_y;
    double diff_cursor_x = current_cursor_x - last_cursor_x;
    double diff_cursor_y = current_cursor_y - last_cursor_y;
    last_cursor_x = current_cursor_x;
    last_cursor_y = current_cursor_y;
    if (glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_LEFT)) {
      pitch += glm::radians(100.0f) * 1e-3f * -float(diff_cursor_y);
      yaw += glm::radians(100.0f) * 1e-3f * float(diff_cursor_x);
      pitch =
          glm::clamp(pitch, -glm::pi<float>() * 0.5f, glm::pi<float>() * 0.5f);
    }

    scene_settings.camera_position +=
        glm::vec3(camera_to_world * glm::vec4(move, 0.0f));
    scene_->SetSceneSettings(scene_settings);
  }

  // Set FPS on window title
  static int frame_count = 0;
  static float time_count = 0.0f;
  frame_count++;
  time_count += delta_time;
  if (time_count >= 1.0f) {
    glfwSetWindowTitle(window_,
                       fmt::format("{} FPS: {}", name_, frame_count).c_str());
    frame_count = 0;
    time_count = 0.0f;
  }
}

void SceneViewer::OnRender() {
  core_->BeginFrame();
  auto command_buffer = core_->CommandBuffer();

  // Begin render pass
  VkRenderPassBeginInfo render_pass_begin_info{};
  render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_begin_info.renderPass = render_pass_->Handle();
  render_pass_begin_info.framebuffer = framebuffer_->Handle();
  render_pass_begin_info.renderArea.offset = {0, 0};
  render_pass_begin_info.renderArea.extent = framebuffer_image_->Extent();

  std::array<VkClearValue, 3> clear_values{};
  clear_values[0].color = {0.6f, 0.7f, 0.8f, 1.0f};
  clear_values[1].depthStencil = {1.0f, 0};
  clear_values[2].color = {0.0f, 0.0f, 0.0f, 1.0f};
  render_pass_begin_info.clearValueCount =
      static_cast<uint32_t>(clear_values.size());
  render_pass_begin_info.pClearValues = clear_values.data();
  vkCmdBeginRenderPass(command_buffer->Handle(), &render_pass_begin_info,
                       VK_SUBPASS_CONTENTS_INLINE);

  // Bind pipeline
  vkCmdBindPipeline(command_buffer->Handle(), VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipeline_->Handle());

  // Bind descriptor sets
  auto descriptor_set = descriptor_sets_[core_->CurrentFrame()]->Handle();
  vkCmdBindDescriptorSets(
      command_buffer->Handle(), VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline_layout_->Handle(), 0, 1, &descriptor_set, 0, nullptr);

  // Set viewport and scissor
  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = framebuffer_image_->Extent().width;
  viewport.height = framebuffer_image_->Extent().height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(command_buffer->Handle(), 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = framebuffer_image_->Extent();
  vkCmdSetScissor(command_buffer->Handle(), 0, 1, &scissor);

  // Draw
  vkCmdDraw(command_buffer->Handle(), 6, 1, 0, 0);

  // End render pass
  vkCmdEndRenderPass(command_buffer->Handle());

  if (window_) {
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
    vkCmdCopyImage(command_buffer->Handle(), framebuffer_image_->Handle(),
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
  }

  core_->EndFrame();
}

void SceneViewer::OnInit() {
  // Create uniform buffers
  camera_object_buffers_.resize(core_->MaxFramesInFlight());
  for (size_t i = 0; i < core_->MaxFramesInFlight(); i++) {
    camera_object_buffers_[i] = std::make_unique<vulkan::Buffer>(
        core_.get(), sizeof(CameraObject),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);
  }

  // Create host uniform buffers
  camera_object_staging_buffer_ = std::make_unique<vulkan::Buffer>(
      core_.get(), sizeof(CameraObject),
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VMA_MEMORY_USAGE_CPU_ONLY);

  triangle_buffer_ = std::make_unique<vulkan::Buffer>(
      core_.get(), scene_->GetTriangleBuffer().size() * sizeof(Triangle),
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VMA_MEMORY_USAGE_GPU_ONLY);

  triangle_material_buffer_ = std::make_unique<vulkan::Buffer>(
      core_.get(),
      scene_->GetTriangleMaterialBuffer().size() * sizeof(Material),
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VMA_MEMORY_USAGE_GPU_ONLY);

  sphere_buffer_ = std::make_unique<vulkan::Buffer>(
      core_.get(), scene_->GetSphereBuffer().size() * sizeof(Sphere),
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VMA_MEMORY_USAGE_GPU_ONLY);

  sphere_material_buffer_ = std::make_unique<vulkan::Buffer>(
      core_.get(), scene_->GetSphereMaterialBuffer().size() * sizeof(Material),
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VMA_MEMORY_USAGE_GPU_ONLY);

  point_light_buffer_ = std::make_unique<vulkan::Buffer>(
      core_.get(), scene_->GetPointLightBuffer().size() * sizeof(PointLight),
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VMA_MEMORY_USAGE_GPU_ONLY);

  vulkan::UploadBuffer(
      triangle_buffer_.get(),
      reinterpret_cast<const void *>(scene_->GetTriangleBuffer().data()),
      scene_->GetTriangleBuffer().size() * sizeof(Triangle));

  vulkan::UploadBuffer(
      triangle_material_buffer_.get(),
      reinterpret_cast<const void *>(
          scene_->GetTriangleMaterialBuffer().data()),
      scene_->GetTriangleMaterialBuffer().size() * sizeof(Material));

  vulkan::UploadBuffer(
      sphere_buffer_.get(),
      reinterpret_cast<const void *>(scene_->GetSphereBuffer().data()),
      scene_->GetSphereBuffer().size() * sizeof(Sphere));

  vulkan::UploadBuffer(
      sphere_material_buffer_.get(),
      reinterpret_cast<const void *>(scene_->GetSphereMaterialBuffer().data()),
      scene_->GetSphereMaterialBuffer().size() * sizeof(Material));

  vulkan::UploadBuffer(
      point_light_buffer_.get(),
      reinterpret_cast<const void *>(scene_->GetPointLightBuffer().data()),
      scene_->GetPointLightBuffer().size() * sizeof(PointLight));

  vertex_shader_ = std::make_unique<vulkan::ShaderModule>(
      core_.get(), vulkan::CompileGLSLToSPIRV(
                       GetShaderCode("shaders/raytracing_shader.vert"),
                       VK_SHADER_STAGE_VERTEX_BIT));

  fragment_shader_ = std::make_unique<vulkan::ShaderModule>(
      core_.get(), vulkan::CompileGLSLToSPIRV(
                       GetShaderCode("shaders/raytracing_shader.frag"),
                       VK_SHADER_STAGE_FRAGMENT_BIT));

  // Create descriptor pool and sets
  descriptor_pool_ = std::make_unique<vulkan::DescriptorPool>(core_.get());
  descriptor_set_layout_ = std::make_unique<vulkan::DescriptorSetLayout>(
      core_.get(), std::vector<VkDescriptorSetLayoutBinding>{
                       {
                           0,
                           VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                           1,
                           VK_SHADER_STAGE_FRAGMENT_BIT,
                           nullptr,
                       },
                       {
                           1,
                           VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                           1,
                           VK_SHADER_STAGE_FRAGMENT_BIT,
                           nullptr,
                       },
                       {
                           2,
                           VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                           1,
                           VK_SHADER_STAGE_FRAGMENT_BIT,
                           nullptr,
                       },
                       {
                           3,
                           VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                           1,
                           VK_SHADER_STAGE_FRAGMENT_BIT,
                           nullptr,
                       },
                       {
                           4,
                           VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                           1,
                           VK_SHADER_STAGE_FRAGMENT_BIT,
                           nullptr,
                       },
                       {
                           5,
                           VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                           1,
                           VK_SHADER_STAGE_FRAGMENT_BIT,
                           nullptr,
                       },
                   });
  descriptor_sets_.resize(core_->MaxFramesInFlight());
  for (size_t i = 0; i < core_->MaxFramesInFlight(); i++) {
    descriptor_sets_[i] = std::make_unique<vulkan::DescriptorSet>(
        core_.get(), descriptor_pool_.get(), descriptor_set_layout_.get());
    // Update descriptor set
    VkDescriptorBufferInfo buffer_infos[6]{
        {camera_object_buffers_[i]->Handle(), 0, sizeof(CameraObject)},
        {triangle_buffer_->Handle(), 0,
         scene_->GetTriangleBuffer().size() * sizeof(Triangle)},
        {triangle_material_buffer_->Handle(), 0,
         scene_->GetTriangleMaterialBuffer().size() * sizeof(Material)},
        {sphere_buffer_->Handle(), 0,
         scene_->GetSphereBuffer().size() * sizeof(Sphere)},
        {sphere_material_buffer_->Handle(), 0,
         scene_->GetSphereMaterialBuffer().size() * sizeof(Material)},
        {point_light_buffer_->Handle(), 0,
         scene_->GetPointLightBuffer().size() * sizeof(PointLight)},
    };

    VkWriteDescriptorSet descriptor_writes[6]{
        {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            descriptor_sets_[i]->Handle(),
            0,
            0,
            1,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            nullptr,
            &buffer_infos[0],
            nullptr,
        },
        {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            descriptor_sets_[i]->Handle(),
            1,
            0,
            1,
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            nullptr,
            &buffer_infos[1],
            nullptr,
        },
        {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            descriptor_sets_[i]->Handle(),
            2,
            0,
            1,
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            nullptr,
            &buffer_infos[2],
            nullptr,
        },
        {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            descriptor_sets_[i]->Handle(),
            3,
            0,
            1,
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            nullptr,
            &buffer_infos[3],
            nullptr,
        },
        {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            descriptor_sets_[i]->Handle(),
            4,
            0,
            1,
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            nullptr,
            &buffer_infos[4],
            nullptr,
        },
        {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            descriptor_sets_[i]->Handle(),
            5,
            0,
            1,
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            nullptr,
            &buffer_infos[5],
            nullptr,
        },
    };

    vkUpdateDescriptorSets(core_->Device()->Handle(), 6, descriptor_writes, 0,
                           nullptr);
  }

  // Create render pass, in C++17 we can use std::vector::data() directly
  std::vector<VkAttachmentDescription> attachment_descriptions = {
      {
          0,
          window_ ? core_->SwapChain()->Format() : VK_FORMAT_R8G8B8A8_UNORM,
          VK_SAMPLE_COUNT_1_BIT,
          VK_ATTACHMENT_LOAD_OP_DONT_CARE,
          VK_ATTACHMENT_STORE_OP_STORE,
          VK_ATTACHMENT_LOAD_OP_DONT_CARE,
          VK_ATTACHMENT_STORE_OP_DONT_CARE,
          VK_IMAGE_LAYOUT_UNDEFINED,
          VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      },
      {
          0,
          VK_FORMAT_D32_SFLOAT,
          VK_SAMPLE_COUNT_1_BIT,
          VK_ATTACHMENT_LOAD_OP_CLEAR,
          VK_ATTACHMENT_STORE_OP_DONT_CARE,
          VK_ATTACHMENT_LOAD_OP_DONT_CARE,
          VK_ATTACHMENT_STORE_OP_DONT_CARE,
          VK_IMAGE_LAYOUT_UNDEFINED,
          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
      },
  };

  std::vector<VkAttachmentReference> color_attachment_references = {
      VkAttachmentReference{
          0,
          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      },
  };

  std::optional<VkAttachmentReference> depth_attachment_references = {
      VkAttachmentReference{
          1,
          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
      },
  };

  // Create render pass
  render_pass_ = std::make_unique<vulkan::RenderPass>(
      core_.get(), attachment_descriptions, color_attachment_references,
      depth_attachment_references);

  // Create pipeline layout
  pipeline_layout_ = std::make_unique<vulkan::PipelineLayout>(
      core_.get(),
      std::vector<VkDescriptorSetLayout>{descriptor_set_layout_->Handle()});

  // Create pipeline
  vulkan::PipelineSettings pipeline_settings(render_pass_.get(),
                                             pipeline_layout_.get());
  pipeline_settings.AddShaderStage(vertex_shader_.get(),
                                   VK_SHADER_STAGE_VERTEX_BIT);
  pipeline_settings.AddShaderStage(fragment_shader_.get(),
                                   VK_SHADER_STAGE_FRAGMENT_BIT);
  pipeline_settings.SetPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
  pipeline_settings.SetCullMode(VK_CULL_MODE_NONE);

  pipeline_ =
      std::make_unique<vulkan::Pipeline>(core_.get(), pipeline_settings);

  BuildScreenFrameObjects();
}

void SceneViewer::BuildScreenFrameObjects() {
  framebuffer_.reset();
  framebuffer_image_.reset();
  depth_image_.reset();

  VkExtent2D extent = window_ ? core_->SwapChain()->Extent()
                              : VkExtent2D{static_cast<uint32_t>(width_),
                                           static_cast<uint32_t>(height_)};
  VkFormat format =
      window_ ? core_->SwapChain()->Format() : VK_FORMAT_R8G8B8A8_UNORM;

  depth_image_ = std::make_unique<vulkan::Image>(
      core_.get(), VK_FORMAT_D32_SFLOAT, extent,
      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT,
      VK_SAMPLE_COUNT_1_BIT);
  framebuffer_image_ = std::make_unique<vulkan::Image>(
      core_.get(), format, extent,
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
      VK_IMAGE_ASPECT_COLOR_BIT, VK_SAMPLE_COUNT_1_BIT);

  framebuffer_ = std::make_unique<vulkan::Framebuffer>(
      core_.get(), extent, render_pass_->Handle(),
      std::vector<VkImageView>{framebuffer_image_->ImageView(),
                               depth_image_->ImageView()});
}

void SceneViewer::OnClose() {
  // Release resources in reverse order of creation
  core_->Device()->WaitIdle();

  depth_image_.reset();
  framebuffer_image_.reset();
  framebuffer_.reset();
  pipeline_.reset();
  pipeline_layout_.reset();
  render_pass_.reset();
  descriptor_sets_.clear();
  descriptor_set_layout_.reset();
  descriptor_pool_.reset();
  vertex_shader_.reset();
  fragment_shader_.reset();
  camera_object_staging_buffer_.reset();
  camera_object_buffers_.clear();
  point_light_buffer_.reset();
  sphere_material_buffer_.reset();
  sphere_buffer_.reset();
  triangle_material_buffer_.reset();
  triangle_buffer_.reset();

  core_.reset();
  if (window_) {
    glfwDestroyWindow(window_);
    glfwTerminate();
  }
}
