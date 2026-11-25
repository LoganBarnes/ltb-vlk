// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "producer.hpp"

// project
#include "common.hpp"
#include "ltb/vlk/check.hpp"
#include "ltb/vlk/ltb_vlk_config.hpp"

// external
#include <spdlog/spdlog.h>

// standard
#include <algorithm>
#include <array>
#include <iterator>
#include <limits>
#include <thread>

namespace ltb::share
{
namespace
{

struct PatternParams
{
    float32 time = 0.0F;
    float32 hue  = 0.0F;
};

auto color_subresource_range( ) -> vk::ImageSubresourceRange
{
    return vk::ImageSubresourceRange{ }
        .setAspectMask( vk::ImageAspectFlagBits::eColor )
        .setLevelCount( 1U )
        .setLayerCount( 1U );
}

} // namespace

ShareProducer::ImageSlot::ImageSlot( vlk::Device& device, vlk::RenderPass& render_pass )
    : shared( device )
    , view( device )
    , framebuffer( device, render_pass )
{
}

ShareProducer::ShareProducer( ProducerSettings settings )
    : settings_( std::move( settings ) )
{
}

auto ShareProducer::initialize( ) -> utils::Result< exec::UpdateLoopStatus >
{
    if ( this->is_initialized( ) )
    {
        return exec::UpdateLoopStatus{ };
    }

    // Headless: no surface or swapchain, but the images' memory must be shareable.
    LTB_CHECK( gpu_.initialize( {
        .device = { .extensions = vlk::shared_image_device_extensions( ) },
    } ) );
    spdlog::
        info( "Rendering on {}", std::string{ gpu_.physical_device( ).properties( ).deviceName } );

    LTB_CHECK_VALID( gpu_.device( ).queues( ).contains( vlk::QueueType::Graphics ) );
    graphics_queue_ = gpu_.device( ).queues( ).at( vlk::QueueType::Graphics );
    queue_family_   = gpu_.physical_device( ).queue_families( ).at( vlk::QueueType::Graphics );

    LTB_CHECK( this->initialize_graphics( ) );
    LTB_CHECK( this->initialize_slots( ) );
    LTB_CHECK( this->connect_to_display( ) );

    start_time_      = Clock::now( );
    next_frame_time_ = start_time_;
    initialized_     = true;

    return exec::UpdateLoopStatus{ };
}

auto ShareProducer::is_initialized( ) const -> bool
{
    return initialized_;
}

auto ShareProducer::fixed_step_update( exec::UpdateLoopStatus const& status )
    -> exec::UpdateRequests
{
    return status.requests;
}

auto ShareProducer::frame_update( exec::UpdateLoopStatus const& status ) -> exec::UpdateRequests
{
    auto requests = status.requests;

    if ( auto const keep_running = this->step( ); !keep_running )
    {
        // Reported (and turned into a failing exit code) by clean_up().
        failure_                  = keep_running.error( );
        requests.exit_update_loop = true;
    }
    else if ( !keep_running.value( ) )
    {
        requests.exit_update_loop = true;
    }

    return requests;
}

auto ShareProducer::step( ) -> utils::Result< bool >
{
    LTB_CHECK( auto const display_connected, this->receive_messages( ) );
    if ( !display_connected )
    {
        spdlog::info( "The display closed the connection. Exiting." );
        return false;
    }
    if ( stop_requested( ) )
    {
        spdlog::info( "Stopping." );
        return false;
    }

    // Don't render faster than requested.
    if ( auto const now = Clock::now( ); now < next_frame_time_ )
    {
        std::this_thread::sleep_until( next_frame_time_ );
    }

    // Both slots may still be with the display. Check again shortly.
    auto const free_slot = std::ranges::find( slots_, false, &ImageSlot::shared_with_display );
    if ( slots_.end( ) == free_slot )
    {
        std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
        return true;
    }

    auto const slot_index = static_cast< uint32 >( std::distance( slots_.begin( ), free_slot ) );
    LTB_CHECK( auto const frame_sent, this->render( slot_index ) );
    if ( !frame_sent )
    {
        // The display closed the connection while this frame was rendering. If it rejected
        // this producer, it said why first.
        LTB_CHECK( this->receive_messages( ) );
        spdlog::info( "The display closed the connection. Exiting." );
        return false;
    }

    auto const frame_period = std::chrono::duration_cast< Clock::duration >(
        std::chrono::duration< float64 >( 1.0 / settings_.frames_per_second )
    );
    next_frame_time_ = std::max( next_frame_time_ + frame_period, Clock::now( ) );

    return true;
}

auto ShareProducer::on_resize( glm::ivec2 const size ) -> utils::Result< void >
{
    utils::ignore( size );
    return utils::success( );
}

auto ShareProducer::clean_up( ) -> utils::Result< void >
{
    if ( gpu_.device( ).is_initialized( ) )
    {
        VK_CHECK( gpu_.device( ).get( ).waitIdle( ) );
    }
    if ( failure_.has_value( ) )
    {
        return tl::make_unexpected( failure_.value( ) );
    }
    return utils::success( );
}

auto ShareProducer::initialize_graphics( ) -> utils::Result< void >
{
    // One color attachment, left in `shared_image_layout` for the hand-off to the display.
    auto render_pass_settings = vlk::render_pass_settings_2d( shared_image_format );
    render_pass_settings.attachments.front( ).setFinalLayout( shared_image_layout );
    LTB_CHECK( render_pass_.initialize( std::move( render_pass_settings ) ) );

    auto pipeline_settings = vlk::GraphicsPipelineSettings{ };
    pipeline_settings.rasterizer.setCullMode( vk::CullModeFlagBits::eNone );
    pipeline_settings.depth_stencil = std::nullopt;

    LTB_CHECK( pipeline_.initialize( {
        .shader_modules = {
            {
                .spirv_file = vlk::config::spirv_dir_path( ) / "share" / "fullscreen.vert.spv",
                .stage      = vk::ShaderStageFlagBits::eVertex,
            },
            {
                .spirv_file = vlk::config::spirv_dir_path( ) / "share" / "pattern.frag.spv",
                .stage      = vk::ShaderStageFlagBits::eFragment,
            },
        },
        .descriptor_set_count   = 1U,
        .uniform_push_constants = {
            vk::PushConstantRange{ }
                .setStageFlags( vk::ShaderStageFlagBits::eFragment )
                .setSize( sizeof( PatternParams ) ),
        },
        .pipeline = std::move( pipeline_settings ),
    } ) );

    LTB_CHECK( command_pool_.initialize( ) );
    LTB_CHECK( command_buffer_.initialize( ) );
    LTB_CHECK( render_fence_.initialize( { .flags = {} } ) );

    return utils::success( );
}

auto ShareProducer::initialize_slots( ) -> utils::Result< void >
{
    for ( auto i = 0U; i < image_slot_count; ++i )
    {
        auto& slot = slots_.emplace_back( gpu_.device( ), render_pass_ );

        LTB_CHECK( slot.shared.create( shared_image_settings( settings_.extent ) ) );

        LTB_CHECK( slot.view.initialize( {
            .image  = slot.shared.image( ).get( ),
            .format = shared_image_format,
        } ) );
        LTB_CHECK( slot.framebuffer.initialize( {
            .attachments = { slot.view.get( ) },
            .extent      = settings_.extent,
        } ) );
    }

    return utils::success( );
}

auto ShareProducer::connect_to_display( ) -> utils::Result< void >
{
    auto channel = ipc::Channel::connect( settings_.display_address );
    if ( !channel )
    {
        return LTB_MAKE_UNEXPECTED_ERROR(
            "Couldn't connect to a display at '{}'. Start one first with --display.\n{}",
            settings_.display_address,
            channel.error( ).error_message( )
        );
    }
    display_ = std::move( channel.value( ) );

    // A new handle for each slot's memory. They're closed once sent; the display gets its
    // own copies.
    auto handles = std::vector< ipc::NativeHandle >{ };
    for ( auto const& slot : slots_ )
    {
        LTB_CHECK( auto handle, slot.shared.export_handle( ) );
        handles.push_back( std::move( handle ) );
    }

    LTB_CHECK(
        auto const hello_status,
        display_.send_value(
            HelloMessage{
                .slot_count = static_cast< uint32 >( slots_.size( ) ),
                .image      = slots_.front( ).shared.info( ),
                .device     = gpu_.physical_device( ).identity( ),
                .process_id = current_process_id( ),
            },
            handles
        )
    );
    LTB_CHECK_VALID(
        ipc::SendStatus::Sent == hello_status,
        "the display closed the connection before it could be set up"
    );

    spdlog::info( "Sharing output with the display at '{}'", settings_.display_address );
    return utils::success( );
}

auto ShareProducer::receive_messages( ) -> utils::Result< bool >
{
    auto buffer
        = std::array< std::byte, std::max( sizeof( FrameMessage ), sizeof( RejectedMessage ) ) >{ };

    while ( true )
    {
        LTB_CHECK( auto const received, display_.receive( buffer ) );

        switch ( received.status )
        {
            case ipc::ReceiveStatus::WouldBlock:
                return true;
            case ipc::ReceiveStatus::Closed:
                return false;
            case ipc::ReceiveStatus::Message:
                break;
        }

        auto const bytes = std::span< std::byte const >{ buffer }.first( received.message.size );

        if ( auto const rejected = read_message< RejectedMessage >( bytes, MessageType::Rejected ) )
        {
            return LTB_MAKE_UNEXPECTED_ERROR(
                "The display rejected this producer: {}",
                rejection_reason( rejected.value( ) )
            );
        }

        auto const message = read_message< FrameMessage >( bytes, MessageType::SlotReleased );
        LTB_CHECK_VALID( message.has_value( ), "unexpected message from the display" );
        LTB_CHECK_VALID( message->slot < slots_.size( ) );

        slots_[ message->slot ].shared_with_display = false;
    }
}

auto ShareProducer::render( uint32 const slot_index ) -> utils::Result< bool >
{
    LTB_CHECK_VALID( slot_index < slots_.size( ) );
    auto& slot = slots_[ slot_index ];

    auto const& commands = command_buffer_.get( );
    VK_CHECK( commands.reset( ) );
    VK_CHECK( commands.begin(
        vk::CommandBufferBeginInfo{ }.setFlags( vk::CommandBufferUsageFlagBits::eOneTimeSubmit )
    ) );

    auto const render_area = vk::Rect2D{ }.setExtent( settings_.extent );
    auto const clear_value = vk::ClearValue{ }.setColor( { 0.0F, 0.0F, 0.0F, 1.0F } );

    // The render pass starts from an undefined layout, so the previous contents (and any
    // ownership the display had) don't matter: the whole image is redrawn.
    commands.beginRenderPass(
        vk::RenderPassBeginInfo{ }
            .setRenderPass( render_pass_.get( ) )
            .setFramebuffer( slot.framebuffer.get( ) )
            .setRenderArea( render_area )
            .setClearValues( clear_value ),
        vk::SubpassContents::eInline
    );

    pipeline_.bind( commands );
    commands.setViewport(
        0U,
        vk::Viewport{ }
            .setWidth( static_cast< float32 >( settings_.extent.width ) )
            .setHeight( static_cast< float32 >( settings_.extent.height ) )
            .setMaxDepth( 1.0F )
    );
    commands.setScissor( 0U, render_area );

    auto const elapsed = std::chrono::duration< float32 >( Clock::now( ) - start_time_ ).count( );
    auto const params  = PatternParams{ .time = elapsed, .hue = settings_.hue };
    commands.pushConstants(
        pipeline_.pipeline_layout( ).get( ),
        vk::ShaderStageFlagBits::eFragment,
        0U,
        sizeof( params ),
        &params
    );

    constexpr auto fullscreen_triangle_vertices = 3U;
    commands.draw( fullscreen_triangle_vertices, 1U, 0U, 0U );

    commands.endRenderPass( );

    // Release ownership to the external queue family. The display acquires it with the
    // matching barrier before reading.
    commands.pipelineBarrier(
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::PipelineStageFlagBits::eBottomOfPipe,
        { },
        { },
        { },
        vk::ImageMemoryBarrier{ }
            .setImage( slot.shared.image( ).get( ) )
            .setSubresourceRange( color_subresource_range( ) )
            .setOldLayout( shared_image_layout )
            .setNewLayout( shared_image_layout )
            .setSrcAccessMask( vk::AccessFlagBits::eColorAttachmentWrite )
            .setSrcQueueFamilyIndex( queue_family_ )
            .setDstQueueFamilyIndex( vk::QueueFamilyExternal )
    );

    VK_CHECK( commands.end( ) );

    // Wait for the frame to finish before telling the display it's ready.
    VK_CHECK(
        graphics_queue_
            .submit( vk::SubmitInfo{ }.setCommandBuffers( commands ), render_fence_.get( ) )
    );
    constexpr auto no_timeout = std::numeric_limits< uint64 >::max( );
    VK_CHECK( gpu_.device( ).get( ).waitForFences( render_fence_.get( ), true, no_timeout ) );
    VK_CHECK( gpu_.device( ).get( ).resetFences( render_fence_.get( ) ) );

    LTB_CHECK(
        auto const frame_status,
        display_.send_value(
            FrameMessage{
                .type         = MessageType::FrameReady,
                .slot         = slot_index,
                .frame_number = frame_number_++,
            }
        )
    );
    if ( ipc::SendStatus::Closed == frame_status )
    {
        return false;
    }
    slot.shared_with_display = true;

    return true;
}

} // namespace ltb::share
