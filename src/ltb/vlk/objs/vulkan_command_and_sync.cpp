// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/vlk/objs/vulkan_command_and_sync.hpp"

// project
#include "ltb/vlk/check.hpp"
#include "ltb/vlk/device.hpp"
#include "ltb/vlk/physical_device.hpp"
#include "ltb/vlk/swapchain.hpp"

// external
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/transform.hpp>
#include <spdlog/spdlog.h>

namespace ltb::vlk::objs
{
namespace
{

struct GetSemaphore
{
    auto operator( )( SemaphoreAndStage const& sem_and_stage ) const -> vk::Semaphore
    {
        return sem_and_stage.semaphore;
    }
};

struct GetStage
{
    auto operator( )( SemaphoreAndStage const& sem_and_stage ) const -> vk::PipelineStageFlags
    {
        return sem_and_stage.stage;
    }
};

} // namespace

VulkanCommandAndSync::VulkanCommandAndSync( VulkanGpu& gpu )
    : gpu_( gpu )
{
}

auto VulkanCommandAndSync::initialize( VulkanCommandAndSyncSettings const settings )
    -> utils::Result< void >
{
    if ( this->is_initialized( ) )
    {
        return utils::success( );
    }
    LTB_CHECK_VALID( gpu_.is_initialized( ) );
    LTB_CHECK_VALID( settings.frame_count > 0U );

    LTB_CHECK( command_pool_.initialize( settings.command_pool ) );

    for ( auto frame_index = 0U; frame_index < settings.frame_count; ++frame_index )
    {
        LTB_CHECK( command_buffers_.emplace_back( gpu_.device( ), command_pool_ ).initialize( ) );
        LTB_CHECK( frame_fences_.emplace_back( gpu_.device( ) ).initialize( ) );
        LTB_CHECK( frame_semaphores_.emplace_back( gpu_.device( ) ).initialize( ) );
    }

    for ( auto image_index = 0UL; image_index < settings.image_count; ++image_index )
    {
        LTB_CHECK( image_semaphores_.emplace_back( gpu_.device( ) ).initialize( ) );
    }

    initialized_ = true;

    return utils::success( );
}

auto VulkanCommandAndSync::is_initialized( ) const -> bool
{
    return initialized_;
}

auto VulkanCommandAndSync::start_frame( Swapchain const& swapchain )
    -> utils::Result< std::optional< FrameInfo > >
{
    return start_frame( swapchain, ResetCommandBuffer::Yes );
}

auto VulkanCommandAndSync::start_frame(
    Swapchain const&         swapchain,
    ResetCommandBuffer const reset_command_buffer
) -> utils::Result< std::optional< FrameInfo > >
{
    constexpr auto max_possible_timeout = std::numeric_limits< uint32 >::max( );

    auto const  frame_objects             = this->get_frame_objects( );
    auto const& command_buffer            = frame_objects.command_buffer;
    auto const& in_flight_fence           = frame_objects.frame_fence;
    auto const& image_available_semaphore = frame_objects.frame_semaphore;

    auto const     fences       = std::array{ in_flight_fence };
    constexpr auto wait_for_all = true;
    VK_CHECK( gpu_.device( ).get( ).waitForFences( fences, wait_for_all, max_possible_timeout ) );

    auto const image_index_result = gpu_.device( ).get( ).acquireNextImageKHR(
        swapchain.get( ),
        max_possible_timeout,
        image_available_semaphore
    );
    if ( image_index_result.result == vk::Result::eErrorOutOfDateKHR )
    {
        // Swapchain needs to be recreated. Not a real error.
        return std::nullopt;
    }
    VK_CHECK( auto const image_index, image_index_result );

    VK_CHECK( gpu_.device( ).get( ).resetFences( fences ) );

    if ( ResetCommandBuffer::Yes == reset_command_buffer )
    {
        constexpr auto reset_flags = vk::CommandBufferResetFlags{ };
        VK_CHECK( command_buffer.reset( reset_flags ) );
    }

    return FrameInfo{
        .command_buffer  = command_buffer,
        .frame_fence     = in_flight_fence,
        .frame_index     = frame_index_,
        .image_semaphore = image_available_semaphore,
        .image_index     = image_index,
    };
}

auto VulkanCommandAndSync::start_frame( ) -> utils::Result< std::optional< FrameInfo > >
{
    return start_frame( ResetCommandBuffer::Yes );
}

auto VulkanCommandAndSync::start_frame( ResetCommandBuffer const reset_command_buffer )
    -> utils::Result< std::optional< FrameInfo > >
{
    constexpr auto max_possible_timeout = std::numeric_limits< uint32 >::max( );

    auto const  frame_objects   = this->get_frame_objects( );
    auto const& command_buffer  = frame_objects.command_buffer;
    auto const& in_flight_fence = frame_objects.frame_fence;

    auto const     fences       = std::array{ in_flight_fence };
    constexpr auto wait_for_all = true;
    VK_CHECK( gpu_.device( ).get( ).waitForFences( fences, wait_for_all, max_possible_timeout ) );

    VK_CHECK( gpu_.device( ).get( ).resetFences( fences ) );

    if ( ResetCommandBuffer::Yes == reset_command_buffer )
    {
        constexpr auto reset_flags = vk::CommandBufferResetFlags{ };
        VK_CHECK( command_buffer.reset( reset_flags ) );
    }

    return FrameInfo{
        .command_buffer  = command_buffer,
        .frame_fence     = in_flight_fence,
        .frame_index     = frame_index_,
        .image_semaphore = nullptr,
        .image_index     = std::numeric_limits< uint32 >::max( ),
    };
}

auto VulkanCommandAndSync::get_frame_objects( ) -> FrameObjects
{
    assert( frame_index_ < command_buffers_.size( ) );
    assert( frame_index_ < frame_fences_.size( ) );
    assert( frame_index_ < frame_semaphores_.size( ) );

    return {
        .command_buffer  = command_buffers_.at( frame_index_ ).get( ),
        .frame_fence     = frame_fences_.at( frame_index_ ).get( ),
        .frame_semaphore = frame_semaphores_.at( frame_index_ ).get( ),
    };
}

auto VulkanCommandAndSync::get_frame_objects( uint32 const frame_index )
    -> utils::Result< FrameObjects >
{
    LTB_CHECK_VALID( frame_index < command_buffers_.size( ) );
    LTB_CHECK_VALID( frame_index < frame_fences_.size( ) );
    LTB_CHECK_VALID( frame_index < frame_semaphores_.size( ) );

    return FrameObjects{
        .command_buffer  = command_buffers_[ frame_index ].get( ),
        .frame_fence     = frame_fences_[ frame_index ].get( ),
        .frame_semaphore = frame_semaphores_[ frame_index ].get( ),
    };
}

auto VulkanCommandAndSync::get_present_semaphore( FrameInfo const& frame )
    -> utils::Result< vk::Semaphore >
{
    LTB_CHECK_VALID( frame.image_index < image_semaphores_.size( ) );

    return image_semaphores_[ frame.image_index ].get( );
}

auto VulkanCommandAndSync::end_frame( FrameInfo const& frame, vk::Queue const& submit_queue )
    -> utils::Result< void >
{
    LTB_CHECK( auto const render_finished_semaphore, this->get_present_semaphore( frame ) );

    auto wait_until_signaled = std::vector< SemaphoreAndStage >{
        {
            .semaphore = this->get_frame_objects( ).frame_semaphore,
            .stage     = vk::PipelineStageFlagBits::eColorAttachmentOutput,
        },
    };

    auto signal_when_finished = std::vector{ render_finished_semaphore };

    return this->end_frame(
        frame,
        std::move( wait_until_signaled ),
        std::move( signal_when_finished ),
        submit_queue
    );
}

auto VulkanCommandAndSync::end_frame(
    FrameInfo const&                        frame,
    std::vector< SemaphoreAndStage > const& wait_until_signaled,
    std::vector< vk::Semaphore > const&     signal_when_finished,
    vk::Queue const&                        submit_queue
) -> utils::Result< void >
{
    auto const wait_semaphores = wait_until_signaled | ranges::views::transform( GetSemaphore{ } )
                               | ranges::to< std::vector >( );
    auto const wait_stages = wait_until_signaled | ranges::views::transform( GetStage{ } )
                           | ranges::to< std::vector >( );

    auto const submit_info = vk::SubmitInfo{ }
                                 .setWaitSemaphores( wait_semaphores )
                                 .setWaitDstStageMask( wait_stages )
                                 .setSignalSemaphores( signal_when_finished )
                                 .setCommandBuffers( frame.command_buffer );

    VK_CHECK( submit_queue.submit( submit_info, frame.frame_fence ) );

    return utils::success( );
}

auto VulkanCommandAndSync::present_frame(
    FrameInfo const& frame,
    Swapchain const& swapchain,
    vk::Queue const& present_queue
) -> utils::Result< void >
{
    LTB_CHECK( auto const render_finished_semaphore, this->get_present_semaphore( frame ) );

    auto const present_info = vk::PresentInfoKHR{ }
                                  .setWaitSemaphores( render_finished_semaphore )
                                  .setSwapchains( swapchain.get( ) )
                                  .setImageIndices( frame.image_index );

    switch ( auto const present_result = present_queue.presentKHR( present_info ) )
    {
        using enum vk::Result;
        case eErrorOutOfDateKHR:
        case eSuboptimalKHR:
            // Swapchain needs to be recreated. Not a real error.
            return utils::success( );
        default:
            VK_CHECK( present_result );
            break;
    }

    return utils::success( );
}

auto VulkanCommandAndSync::increment_frame( ) -> void
{
    frame_index_ = compute_next_frame( frame_index_ );
}

auto VulkanCommandAndSync::frame_index( ) const -> uint32
{
    return frame_index_;
}

auto VulkanCommandAndSync::previous_frame( ) const -> uint32
{
    return compute_previous_frame( frame_index_ );
}

auto VulkanCommandAndSync::compute_previous_frame( uint const index ) const -> uint32
{
    auto const frame_count = static_cast< uint32 >( command_buffers_.size( ) );
    return ( ( index + frame_count ) - 1U ) % frame_count;
}

auto VulkanCommandAndSync::compute_next_frame( uint const index ) const -> uint32
{
    auto const frame_count = static_cast< uint32 >( command_buffers_.size( ) );
    return ( index + 1U ) % frame_count;
}

auto VulkanCommandAndSync::command_pool( ) const -> CommandPool const&
{
    return command_pool_;
}

auto VulkanCommandAndSync::command_pool( ) -> CommandPool&
{
    return command_pool_;
}

auto VulkanCommandAndSync::command_buffers( ) const -> std::vector< CommandBuffer > const&
{
    return command_buffers_;
}

auto VulkanCommandAndSync::command_buffers( ) -> std::vector< CommandBuffer >&
{
    return command_buffers_;
}

auto VulkanCommandAndSync::frame_fences( ) const -> std::vector< Fence > const&
{
    return frame_fences_;
}

auto VulkanCommandAndSync::frame_fences( ) -> std::vector< Fence >&
{
    return frame_fences_;
}

auto VulkanCommandAndSync::frame_semaphores( ) const -> std::vector< Semaphore > const&
{
    return frame_semaphores_;
}

auto VulkanCommandAndSync::frame_semaphores( ) -> std::vector< Semaphore >&
{
    return frame_semaphores_;
}

auto VulkanCommandAndSync::image_semaphores( ) const -> std::vector< Semaphore > const&
{
    return image_semaphores_;
}

auto VulkanCommandAndSync::image_semaphores( ) -> std::vector< Semaphore >&
{
    return image_semaphores_;
}

} // namespace ltb::vlk::objs
