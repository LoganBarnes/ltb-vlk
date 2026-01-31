// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/cam/camera_2d.hpp"
#include "ltb/exec/update_loop.hpp"
#include "ltb/gui/imgui_glfw_vulkan_setup.hpp"
#include "ltb/utils/timers.hpp"
#include "ltb/vlk/buffer.hpp"
#include "ltb/vlk/objs/vulkan_buffer.hpp"
#include "ltb/vlk/objs/vulkan_command_and_sync.hpp"
#include "ltb/vlk/objs/vulkan_compute_pipeline.hpp"
#include "ltb/vlk/objs/vulkan_gpu.hpp"
#include "ltb/vlk/objs/vulkan_graphics_pipeline.hpp"

// standard
#include <unordered_set>

namespace ltb
{

// https://vulkan-tutorial.com/Compute_Shader

// Good reference:
// https://github.com/SaschaWillems/Vulkan-Samples/tree/main/samples/api/compute_nbody

struct Particle
{
    glm::vec2 position = { };
    glm::vec2 velocity = { };
    glm::vec4 color    = { };
};

class ParticlesApp
{
public:
    explicit ParticlesApp( window::GlfwContext& glfw_context, window::GlfwWindow& glfw_window );

    auto initialize( ) -> utils::Result< exec::UpdateLoopStatus >;

    [[nodiscard( "Const getter" )]]
    auto is_initialized( ) const -> bool;

    auto fixed_step_update( exec::UpdateLoopStatus const& status ) -> exec::UpdateRequests;
    auto frame_update( exec::UpdateLoopStatus const& status ) -> exec::UpdateRequests;

    auto configure_gui( ) -> void;

    auto on_resize( glm::ivec2 size ) -> utils::Result< void >;
    auto clean_up( ) -> utils::Result< void >;

private:
    window::GlfwContext& glfw_context_;
    window::GlfwWindow&  glfw_window_;

    vlk::objs::VulkanGpu gpu_                        = { glfw_context_, glfw_window_ };
    vk::Queue            graphics_and_compute_queue_ = nullptr;
    vk::Queue            present_queue_              = nullptr;

    vlk::objs::VulkanPresentation presentation_ = { gpu_ };
    gui::ImguiGlfwVulkanSetup     imgui_        = { glfw_window_, gpu_, presentation_ };

    vlk::objs::VulkanComputePipeline compute_              = { gpu_ };
    vlk::objs::VulkanCommandAndSync  compute_cmd_and_sync_ = { gpu_ };
    vlk::objs::VulkanBuffer          gpu_particles_        = { gpu_ };

    vlk::objs::VulkanGraphicsPipeline graphics_              = { gpu_, presentation_ };
    vlk::objs::VulkanCommandAndSync   graphics_cmd_and_sync_ = { gpu_ };

    vlk::objs::VulkanBuffer      camera_ubo_            = { gpu_ };
    cam::Camera2d                camera_                = { };
    std::unordered_set< uint32 > camera_frames_updated_ = { };

    struct UniformBufferObject
    {
        float32 delta_time = 0.0F;
    };

    UniformBufferObject compute_ubo_ = { };

    utils::Timer timer_ = { };

    bool initialized_ = false;

    auto initialize_gpu_presentation( ) -> utils::Result< ParticlesApp* >;
    auto initialize_compute_pipeline( ) -> utils::Result< ParticlesApp* >;
    auto initialize_particles( ) -> utils::Result< ParticlesApp* >;
    auto initialize_display_pipeline( ) -> utils::Result< ParticlesApp* >;
    auto initialize_camera( ) -> utils::Result< ParticlesApp* >;

    auto compute( ) -> utils::Result< void >;
    auto record_compute_commands( vlk::objs::FrameInfo const& frame ) -> utils::Result< void >;

    auto render( ) -> utils::Result< void >;
    auto update_camera_uniforms( vlk::objs::FrameInfo const& frame ) -> utils::Result< void >;
    auto record_render_commands( vlk::objs::FrameInfo const& frame ) -> utils::Result< void >;
};

} // namespace ltb
