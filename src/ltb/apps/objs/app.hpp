// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "app_pipeline.hpp"
#include "ltb/exec/update_loop.hpp"
#include "ltb/vlk/objs/vulkan_buffer.hpp"
#include "ltb/vlk/objs/vulkan_command_and_sync.hpp"
#include "ltb/window/fwd.hpp"

namespace ltb
{

class ObjsApp

{
public:
    explicit ObjsApp( window::GlfwContext& glfw_context, window::GlfwWindow& glfw_window );

    auto initialize( ) -> utils::Result< exec::UpdateLoopStatus >;

    [[nodiscard( "Const getter" )]]
    auto is_initialized( ) const -> bool;

    auto fixed_step_update( exec::UpdateLoopStatus const& status ) -> exec::UpdateRequests;
    auto frame_update( exec::UpdateLoopStatus const& status ) -> exec::UpdateRequests;

    auto on_resize( glm::ivec2 size ) -> utils::Result< void >;
    auto clean_up( ) -> utils::Result< void >;

private:
    window::GlfwContext& glfw_context_;
    window::GlfwWindow&  glfw_window_;

    vlk::objs::VulkanGpu          gpu_            = { glfw_context_, glfw_window_ };
    vk::Queue                     graphics_queue_ = nullptr;
    vk::Queue                     present_queue_  = nullptr;
    vlk::objs::VulkanPresentation presentation_   = { gpu_ };

    vlk::objs::VulkanBuffer         camera_ubo_   = { gpu_ };
    ObjsAppPipeline                 graphics_     = { gpu_, presentation_ };
    vlk::objs::VulkanCommandAndSync cmd_and_sync_ = { gpu_ };

    ObjsAppPipeline::MeshPushConstants* top_model_uniforms_    = nullptr;
    ObjsAppPipeline::MeshPushConstants* bottom_model_uniforms_ = nullptr;

    bool initialized_ = false;

    auto initialize_gpu_presentation( ) -> utils::Result< ObjsApp* >;
    auto initialize_camera_buffers( ) -> utils::Result< ObjsApp* >;
    auto initialize_graphics( ) -> utils::Result< ObjsApp* >;
    auto initialize_command_and_sync( ) -> utils::Result< ObjsApp* >;
    auto set_camera_buffers( ) -> utils::Result< ObjsApp* >;
    auto initialize_vertex_buffer( ) -> utils::Result< ObjsApp* >;

    auto render( ) -> utils::Result< void >;
    auto record_render_commands( vlk::objs::FrameInfo const& frame ) -> utils::Result< void >;
};

} // namespace ltb
