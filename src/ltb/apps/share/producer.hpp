// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/exec/update_loop.hpp"
#include "ltb/ipc/unix_socket.hpp"
#include "ltb/vlk/command_buffer.hpp"
#include "ltb/vlk/command_pool.hpp"
#include "ltb/vlk/device_memory.hpp"
#include "ltb/vlk/fence.hpp"
#include "ltb/vlk/framebuffer.hpp"
#include "ltb/vlk/image.hpp"
#include "ltb/vlk/image_view.hpp"
#include "ltb/vlk/objs/vulkan_gpu.hpp"
#include "ltb/vlk/objs/vulkan_graphics_pipeline.hpp"
#include "ltb/vlk/render_pass.hpp"

// standard
#include <chrono>
#include <deque>
#include <filesystem>
#include <optional>

namespace ltb::share
{

struct ProducerSettings
{
    std::filesystem::path socket_path = { };

    /// \brief The size of the rendered images.
    vk::Extent2D extent = { 512U, 512U };

    /// \brief 0 to 1. Picks the colors of the pattern so producers are easy to tell apart.
    float32 hue = 0.0F;

    float64 frames_per_second = 60.0;
};

/// \brief Renders an animated pattern offscreen (no window) into images whose memory is
///        shared with a display process.
class ShareProducer
{
public:
    explicit ShareProducer( ProducerSettings settings );

    auto initialize( ) -> utils::Result< exec::UpdateLoopStatus >;

    [[nodiscard( "Const getter" )]]
    auto is_initialized( ) const -> bool;

    auto fixed_step_update( exec::UpdateLoopStatus const& status ) -> exec::UpdateRequests;
    auto frame_update( exec::UpdateLoopStatus const& status ) -> exec::UpdateRequests;
    auto on_resize( glm::ivec2 size ) -> utils::Result< void >;
    auto clean_up( ) -> utils::Result< void >;

private:
    using Clock = std::chrono::steady_clock;

    /// \brief One image the producer renders into and the display reads from.
    struct ImageSlot
    {
        explicit ImageSlot( vlk::Device& device, vlk::RenderPass& render_pass );

        vlk::Image        image;
        vlk::DeviceMemory memory;
        vlk::ImageView    view;
        vlk::Framebuffer  framebuffer;

        /// \brief True from when the frame is sent until the display releases it.
        bool shared_with_display = false;
    };

    ProducerSettings settings_;

    vlk::objs::VulkanGpu gpu_            = { };
    vk::Queue            graphics_queue_ = nullptr;
    uint32               queue_family_   = 0U;

    vlk::RenderPass                   render_pass_ = { gpu_.device( ) };
    vlk::objs::VulkanGraphicsPipeline pipeline_    = { gpu_, render_pass_ };

    // A deque keeps the slots in place (they hold references to the device).
    std::deque< ImageSlot > slots_ = { };

    vlk::CommandPool   command_pool_   = { gpu_.device( ), gpu_.physical_device( ) };
    vlk::CommandBuffer command_buffer_ = { gpu_.device( ), command_pool_ };
    vlk::Fence         render_fence_   = { gpu_.device( ) };

    ipc::UnixSocket socket_ = { };

    Clock::time_point start_time_      = { };
    Clock::time_point next_frame_time_ = { };
    uint64            frame_number_    = 0U;

    /// \brief The error that stopped the producer, returned from clean_up() so the process
    ///        exits with a failure code.
    std::optional< utils::Error > failure_ = std::nullopt;

    bool initialized_ = false;

    auto initialize_graphics( ) -> utils::Result< void >;
    auto initialize_slots( ) -> utils::Result< void >;
    auto connect_to_display( ) -> utils::Result< void >;

    /// \brief Handles messages from the display. Returns false once the display has gone.
    auto receive_messages( ) -> utils::Result< bool >;

    /// \brief One iteration: handle messages, then render a frame if a slot is free.
    ///        Returns false when the producer should stop.
    auto step( ) -> utils::Result< bool >;

    /// \brief Renders into a slot and tells the display. Returns false if the display has
    ///        closed the connection.
    auto render( uint32 slot_index ) -> utils::Result< bool >;
};

} // namespace ltb::share
