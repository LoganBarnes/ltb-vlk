// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/exec/update_loop.hpp"
#include "ltb/utils/timers.hpp"
#include "ltb/vlk/buffer.hpp"
#include "ltb/vlk/command_buffer.hpp"
#include "ltb/vlk/command_pool.hpp"
#include "ltb/vlk/descriptor_pool.hpp"
#include "ltb/vlk/descriptor_set_layout.hpp"
#include "ltb/vlk/descriptor_sets.hpp"
#include "ltb/vlk/device.hpp"
#include "ltb/vlk/device_memory.hpp"
#include "ltb/vlk/fence.hpp"
#include "ltb/vlk/framebuffer.hpp"
#include "ltb/vlk/graphics_pipeline.hpp"
#include "ltb/vlk/image.hpp"
#include "ltb/vlk/image_view.hpp"
#include "ltb/vlk/instance.hpp"
#include "ltb/vlk/physical_device.hpp"
#include "ltb/vlk/pipeline_layout.hpp"
#include "ltb/vlk/render_pass.hpp"
#include "ltb/vlk/semaphore.hpp"
#include "ltb/vlk/shader_module.hpp"
#include "ltb/vlk/surface.hpp"
#include "ltb/vlk/swapchain.hpp"
#include "ltb/window/fwd.hpp"

namespace ltb
{

class ApiApp

{
public:
    explicit ApiApp( window::GlfwContext& glfw_context, window::GlfwWindow& glfw_window );

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

    struct VulkanSurfaceGpu
    {
        ApiApp& self;

        vlk::Instance       instance        = { };
        vlk::Surface        surface         = { self.glfw_window_, instance };
        vlk::PhysicalDevice physical_device = { instance };
        vlk::Device         device          = { physical_device };

        vk::Queue graphics_queue = nullptr;
        vk::Queue present_queue  = nullptr;

        vlk::DescriptorPool descriptor_pool = { device };

    } gpu_ = { *this };

    struct Presentation
    {
        ApiApp& self;

        vlk::Swapchain swapchain = {
            self.gpu_.instance,
            self.gpu_.surface,
            self.gpu_.physical_device,
            self.gpu_.device,
        };

        std::vector< vlk::ImageView > swapchain_image_views = { };

        vlk::Image        depth_image        = { self.gpu_.device };
        vlk::DeviceMemory depth_image_memory = { self.gpu_.device };
        vlk::ImageView    depth_image_view   = { self.gpu_.device };

        vlk::RenderPass                 render_pass  = { self.gpu_.device };
        std::vector< vlk::Framebuffer > framebuffers = { };

    } present_ = { *this };

    struct GraphicsPipeline
    {
        ApiApp& self;

        std::vector< vlk::ShaderModule > shader_modules = { };

        vlk::DescriptorSetLayout descriptor_set_layout = { self.gpu_.device };
        vlk::DescriptorSets      descriptor_sets       = {
            self.gpu_.device,
            self.gpu_.descriptor_pool,
        };

        vlk::PipelineLayout   pipeline_layout = { self.gpu_.device };
        vlk::GraphicsPipeline pipeline        = {
            self.gpu_.device,
            self.present_.render_pass,
            shader_modules,
            pipeline_layout,
        };

    } graphics_ = { *this };

    struct CommandAndSynchronization
    {
        ApiApp& self;

        vlk::CommandPool command_pool = {
            self.gpu_.device,
            self.gpu_.physical_device,
        };

        uint32 current_frame_index = 0U;

        std::vector< vlk::CommandBuffer > command_buffers            = { };
        std::vector< vlk::Fence >         in_flight_fences           = { };
        std::vector< vlk::Semaphore >     image_available_semaphores = { };

        std::vector< vlk::Semaphore > render_finished_semaphores = { };

    } cmd_and_sync_ = { *this };

    struct CameraUniformBuffer
    {
        ApiApp& self;

        vlk::MemoryLayout layout      = { };
        vlk::Buffer       buffer      = { self.gpu_.device };
        vlk::DeviceMemory memory      = { self.gpu_.device };
        uint8*            mapped_data = nullptr;

    } camera_uniform_ = { *this };

    struct VertexBufferObject
    {
        ApiApp& self;

        vlk::MemoryRange  position_range = { };
        vlk::MemoryRange  color_range    = { };
        vlk::MemoryRange  index_range    = { };
        vlk::Buffer       buffer         = { self.gpu_.device };
        vlk::DeviceMemory memory         = { self.gpu_.device };

    } vbo_ = { *this };

    utils::Timer timer_ = { };

    bool initialized_ = false;

    auto initialize_vulkan( ) -> utils::Result< ApiApp* >;
    auto initialize_swapchain( ) -> utils::Result< ApiApp* >;
    auto initialize_graphics( ) -> utils::Result< ApiApp* >;
    auto initialize_command_and_sync( ) -> utils::Result< ApiApp* >;
    auto initialize_camera_buffers( ) -> utils::Result< ApiApp* >;
    auto set_camera_buffers( ) -> utils::Result< ApiApp* >;
    auto initialize_vertex_buffer( ) -> utils::Result< ApiApp* >;

    struct FrameInfo
    {
        vk::CommandBuffer command_buffer;
        uint32            image_index;
        uint32            frame_index;
    };

    auto render( ) -> utils::Result< void >;
    auto record_render_commands( FrameInfo const& frame ) -> utils::Result< void >;
};

} // namespace ltb
