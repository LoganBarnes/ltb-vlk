// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "common.hpp"
#include "ltb/exec/update_loop.hpp"
#include "ltb/ipc/channel.hpp"
#include "ltb/vlk/image_view.hpp"
#include "ltb/vlk/objs/vulkan_command_and_sync.hpp"
#include "ltb/vlk/objs/vulkan_gpu.hpp"
#include "ltb/vlk/objs/vulkan_graphics_pipeline.hpp"
#include "ltb/vlk/objs/vulkan_image.hpp"
#include "ltb/vlk/objs/vulkan_presentation.hpp"
#include "ltb/vlk/shared_image.hpp"
#include "ltb/window/fwd.hpp"

// standard
#include <array>
#include <deque>
#include <list>
#include <optional>
#include <vector>

namespace ltb::share
{

struct DisplaySettings
{
    /// \brief Where to listen for producers.
    ipc::ChannelAddress address = { };
};

/// \brief Opens a window and shows the latest frame from every connected producer, each in
///        its own tile.
class ShareDisplay
{
public:
    ShareDisplay(
        window::GlfwContext& glfw_context,
        window::GlfwWindow&  glfw_window,
        DisplaySettings      settings
    );

    auto initialize( ) -> utils::Result< exec::UpdateLoopStatus >;

    [[nodiscard( "Const getter" )]]
    auto is_initialized( ) const -> bool;

    auto fixed_step_update( exec::UpdateLoopStatus const& status ) -> exec::UpdateRequests;
    auto frame_update( exec::UpdateLoopStatus const& status ) -> exec::UpdateRequests;
    auto on_resize( glm::ivec2 size ) -> utils::Result< void >;
    auto clean_up( ) -> utils::Result< void >;

private:
    struct Producer
    {
        Producer( vlk::objs::VulkanGpu& gpu, ipc::Channel connection, uint64 producer_id );

        uint64       id;
        ipc::Channel channel;

        /// \brief Set once the producer has introduced itself and its memory was imported.
        std::optional< HelloMessage > hello = std::nullopt;

        /// \brief The producer's images, backed by memory imported from the producer.
        std::deque< vlk::SharedImage > slots = { };

        /// \brief The display's own copy of the latest frame, which it samples when drawing.
        ///        Copying lets the producer keep rendering while the display draws.
        vlk::objs::VulkanImage texture;
        vlk::ImageView         texture_view;
        vk::ImageLayout        texture_layout = vk::ImageLayout::eUndefined;

        /// \brief Which descriptor set (and so which texture binding) this producer uses.
        std::optional< uint32 > tile = std::nullopt;

        /// \brief The newest finished frame that hasn't been copied yet.
        std::optional< uint32 > ready_slot = std::nullopt;

        bool has_frame    = false;
        bool disconnected = false;
    };

    /// \brief A slot to hand back to its producer once the GPU is done copying from it.
    struct PendingRelease
    {
        uint64 producer_id = 0U;
        uint32 slot        = 0U;
    };

    window::GlfwContext& glfw_context_;
    window::GlfwWindow&  glfw_window_;
    DisplaySettings      settings_;

    vlk::objs::VulkanGpu gpu_             = { glfw_context_, glfw_window_ };
    vk::Queue            graphics_queue_  = nullptr;
    vk::Queue            present_queue_   = nullptr;
    uint32               graphics_family_ = 0U;

    vlk::objs::VulkanPresentation     presentation_ = { gpu_ };
    vlk::objs::VulkanGraphicsPipeline pipeline_     = { gpu_, presentation_ };
    vlk::objs::VulkanCommandAndSync   cmd_and_sync_ = { gpu_ };
    vk::UniqueSampler                 sampler_      = { };

    ipc::ChannelListener listener_ = { };

    // A list keeps each producer in place (they hold references to the GPU).
    std::list< Producer >                        producers_         = { };
    std::array< bool, max_producers >            tile_in_use_       = { };
    std::vector< std::vector< PendingRelease > > releases_by_frame_ = { };
    uint64                                       next_producer_id_  = 0U;

    /// \brief The error that stopped the display, returned from clean_up() so the process
    ///        exits with a failure code.
    std::optional< utils::Error > failure_ = std::nullopt;

    bool initialized_ = false;

    auto initialize_graphics( ) -> utils::Result< void >;

    auto accept_connections( ) -> utils::Result< void >;
    auto receive_messages( Producer& producer ) -> utils::Result< void >;
    auto import_producer(
        Producer&                         producer,
        HelloMessage const&               hello,
        std::vector< ipc::NativeHandle >& handles
    ) -> utils::Result< void >;
    auto remove_disconnected_producers( ) -> utils::Result< void >;
    auto send_release( Producer const& producer, uint32 slot ) -> void;
    auto send_releases( uint32 frame_index ) -> void;

    auto render( ) -> utils::Result< void >;
    auto record_copies( vlk::objs::FrameInfo const& frame ) -> void;
    auto record_tiles( vlk::objs::FrameInfo const& frame ) -> void;
    auto rebuild_presentation( vlk::objs::Rebuild rebuild ) -> utils::Result< void >;
};

} // namespace ltb::share
