// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "display.hpp"

// project
#include "ltb/exec/app_defaults.hpp"
#include "ltb/vlk/check.hpp"
#include "ltb/vlk/ltb_vlk_config.hpp"
#include "ltb/window/glfw_context.hpp"
#include "ltb/window/glfw_window.hpp"

// external
#include <spdlog/spdlog.h>

// standard
#include <algorithm>
#include <cmath>
#include <iterator>

namespace ltb::share
{
namespace
{

struct TileParams
{
    glm::vec2 offset = glm::vec2( 0.0F ); ///< Tile center in normalized device coordinates
    glm::vec2 scale  = glm::vec2( 1.0F ); ///< Half the tile's size in NDC
};

auto color_subresource_range( ) -> vk::ImageSubresourceRange
{
    return vk::ImageSubresourceRange{ }
        .setAspectMask( vk::ImageAspectFlagBits::eColor )
        .setLevelCount( 1U )
        .setLayerCount( 1U );
}

auto image_barrier(
    vk::Image const       image,
    vk::ImageLayout const old_layout,
    vk::ImageLayout const new_layout
) -> vk::ImageMemoryBarrier
{
    return vk::ImageMemoryBarrier{ }
        .setImage( image )
        .setSubresourceRange( color_subresource_range( ) )
        .setOldLayout( old_layout )
        .setNewLayout( new_layout )
        .setSrcQueueFamilyIndex( vk::QueueFamilyIgnored )
        .setDstQueueFamilyIndex( vk::QueueFamilyIgnored );
}

/// \brief Validates a producer's hello before importing anything.
auto check_hello(
    HelloMessage const&        hello,
    std::size_t const          handle_count,
    vlk::DeviceIdentity const& display_device
) -> utils::Result< void >
{
    constexpr auto max_image_dimension = 8192U;

    LTB_CHECK_VALID(
        protocol_version == hello.version,
        "the producer was built with a different protocol version"
    );
    LTB_CHECK_VALID( ( hello.slot_count > 0U ) && ( hello.slot_count <= 4U ) );
    LTB_CHECK_VALID(
        hello.slot_count == handle_count,
        "expected one memory handle per image slot"
    );

    auto const& image = hello.image.settings;
    LTB_CHECK_VALID( ( image.extent.width > 0U ) && ( image.extent.width <= max_image_dimension ) );
    LTB_CHECK_VALID(
        ( image.extent.height > 0U ) && ( image.extent.height <= max_image_dimension )
    );
    LTB_CHECK_VALID( shared_image_format == image.format, "unexpected image format" );
    LTB_CHECK_VALID(
        static_cast< bool >( image.usage & vk::ImageUsageFlagBits::eTransferSrc ),
        "the display copies from the images, so they need transfer-source usage"
    );
    LTB_CHECK_VALID(
        hello.device == display_device,
        "the producer is rendering on a different GPU or driver, so its memory can't be shared"
    );
    return utils::success( );
}

} // namespace

ShareDisplay::Producer::Producer(
    vlk::objs::VulkanGpu& gpu,
    ipc::Channel          connection,
    uint64 const          producer_id
)
    : id( producer_id )
    , channel( std::move( connection ) )
    , texture( gpu )
    , texture_view( gpu.device( ) )
{
}

ShareDisplay::ShareDisplay(
    window::GlfwContext& glfw_context,
    window::GlfwWindow&  glfw_window,
    DisplaySettings      settings
)
    : glfw_context_( glfw_context )
    , glfw_window_( glfw_window )
    , settings_( std::move( settings ) )
{
}

auto ShareDisplay::initialize( ) -> utils::Result< exec::UpdateLoopStatus >
{
    if ( this->is_initialized( ) )
    {
        return exec::UpdateLoopStatus{ };
    }
    LTB_CHECK_VALID( glfw_context_.is_initialized( ) );
    LTB_CHECK_VALID( glfw_window_.is_initialized( ) );

    // The default extensions (swapchain) plus sharing image memory between processes.
    auto gpu_settings = vlk::objs::VulkanGpuSettings{ };
    std::ranges::copy(
        vlk::shared_image_device_extensions( ),
        std::back_inserter( gpu_settings.device.extensions )
    );
    LTB_CHECK( gpu_.initialize( std::move( gpu_settings ) ) );
    spdlog::
        info( "Displaying on {}", std::string( gpu_.physical_device( ).properties( ).deviceName.data( ) ) );

    auto const& queues = gpu_.device( ).queues( );
    LTB_CHECK_VALID( queues.contains( vlk::QueueType::Graphics ) );
    LTB_CHECK_VALID( queues.contains( vlk::QueueType::Surface ) );
    graphics_queue_  = queues.at( vlk::QueueType::Graphics );
    present_queue_   = queues.at( vlk::QueueType::Surface );
    graphics_family_ = gpu_.physical_device( ).queue_families( ).at( vlk::QueueType::Graphics );

    LTB_CHECK( presentation_.initialize( { } ) );
    LTB_CHECK( this->initialize_graphics( ) );

    LTB_CHECK( listener_, ipc::ChannelListener::listen( settings_.address ) );
    spdlog::info(
        "Waiting for producers at '{}'. Start them with --share (and the same --socket).",
        settings_.address
    );

    initialized_ = true;

    return exec::UpdateLoopStatus{ };
}

auto ShareDisplay::is_initialized( ) const -> bool
{
    return initialized_;
}

auto ShareDisplay::fixed_step_update( exec::UpdateLoopStatus const& status ) -> exec::UpdateRequests
{
    return status.requests;
}

auto ShareDisplay::frame_update( exec::UpdateLoopStatus const& status ) -> exec::UpdateRequests
{
    auto requests = status.requests;

    if ( stop_requested( ) )
    {
        requests.exit_update_loop = true;
        return requests;
    }

    auto result = this->accept_connections( );
    for ( auto& producer : producers_ )
    {
        // A misbehaving producer is disconnected; it doesn't stop the display.
        if ( auto received = this->receive_messages( producer ); !received )
        {
            auto const& reason = received.error( ).error_message( );
            spdlog::warn( "Disconnecting producer {}: {}", producer.id, reason );

            // Tell the producer why, so it can report it. Best effort: it may already be gone.
            utils::ignore( producer.channel.send_value( make_rejected_message( reason ) ) );
            producer.disconnected = true;
        }
    }

    result = result
                 .and_then( [ this ]( ) -> utils::Result< void > {
                     return this->remove_disconnected_producers( );
                 } )
                 .and_then( [ this ]( ) -> utils::Result< void > { return this->render( ); } );

    if ( !result )
    {
        // Reported (and turned into a failing exit code) by clean_up().
        failure_                  = result.error( );
        requests.exit_update_loop = true;
    }

    return requests;
}

auto ShareDisplay::on_resize( glm::ivec2 const size ) -> utils::Result< void >
{
    utils::ignore( size );
    return this->rebuild_presentation( vlk::objs::Rebuild::IfSizeChanged );
}

auto ShareDisplay::clean_up( ) -> utils::Result< void >
{
    VK_CHECK( gpu_.device( ).get( ).waitIdle( ) );
    if ( failure_.has_value( ) )
    {
        return tl::make_unexpected( failure_.value( ) );
    }
    return utils::success( );
}

auto ShareDisplay::initialize_graphics( ) -> utils::Result< void >
{
    auto pipeline_settings = vlk::GraphicsPipelineSettings{ };
    pipeline_settings.rasterizer.setCullMode( vk::CullModeFlagBits::eNone );

    // One descriptor set per tile, each holding that producer's texture.
    LTB_CHECK( pipeline_.initialize( {
        .shader_modules = {
            {
                .spirv_file = vlk::config::spirv_dir_path( ) / "share" / "tile.vert.spv",
                .stage      = vk::ShaderStageFlagBits::eVertex,
            },
            {
                .spirv_file = vlk::config::spirv_dir_path( ) / "share" / "tile.frag.spv",
                .stage      = vk::ShaderStageFlagBits::eFragment,
            },
        },
        .descriptor_set_count = max_producers,
        .uniform_binding_sets = {
            {
                vk::DescriptorSetLayoutBinding{ }
                    .setBinding( 0U )
                    .setDescriptorType( vk::DescriptorType::eCombinedImageSampler )
                    .setDescriptorCount( 1U )
                    .setStageFlags( vk::ShaderStageFlagBits::eFragment ),
            },
        },
        .uniform_push_constants = {
            vk::PushConstantRange{ }
                .setStageFlags( vk::ShaderStageFlagBits::eVertex )
                .setSize( sizeof( TileParams ) ),
        },
        .pipeline = std::move( pipeline_settings ),
    } ) );

    LTB_CHECK( cmd_and_sync_.initialize( {
        .frame_count  = exec::max_frames_in_flight,
        .image_count  = static_cast< uint32 >( presentation_.swapchain( ).images( ).size( ) ),
        .command_pool = { .queue_type = vlk::QueueType::Graphics },
    } ) );
    releases_by_frame_.resize( exec::max_frames_in_flight );

    VK_CHECK(
        sampler_,
        gpu_.device( ).get( ).createSamplerUnique(
            vk::SamplerCreateInfo{ }
                .setMagFilter( vk::Filter::eLinear )
                .setMinFilter( vk::Filter::eLinear )
                .setAddressModeU( vk::SamplerAddressMode::eClampToEdge )
                .setAddressModeV( vk::SamplerAddressMode::eClampToEdge )
                .setAddressModeW( vk::SamplerAddressMode::eClampToEdge )
        )
    );

    return utils::success( );
}

auto ShareDisplay::accept_connections( ) -> utils::Result< void >
{
    while ( true )
    {
        LTB_CHECK( auto connection, listener_.accept( ) );
        if ( !connection.has_value( ) )
        {
            return utils::success( );
        }
        auto& producer
            = producers_
                  .emplace_back( gpu_, std::move( connection.value( ) ), next_producer_id_++ );
        spdlog::debug( "Producer {} connected", producer.id );
    }
}

auto ShareDisplay::receive_messages( Producer& producer ) -> utils::Result< void >
{
    auto buffer
        = std::array< std::byte, std::max( sizeof( HelloMessage ), sizeof( FrameMessage ) ) >{ };

    while ( !producer.disconnected )
    {
        LTB_CHECK( auto received, producer.channel.receive( buffer ) );

        if ( ipc::ReceiveStatus::WouldBlock == received.status )
        {
            break;
        }
        if ( ipc::ReceiveStatus::Closed == received.status )
        {
            spdlog::info( "Producer {} disconnected", producer.id );
            producer.disconnected = true;
            break;
        }

        auto const bytes = std::span< std::byte const >{ buffer }.first( received.message.size );
        auto const type  = peek_message_type( bytes );

        if ( MessageType::Hello == type )
        {
            LTB_CHECK_VALID( !producer.hello.has_value( ), "received a second hello" );
            auto const hello = read_message< HelloMessage >( bytes, MessageType::Hello );
            LTB_CHECK_VALID( hello.has_value( ), "malformed hello" );
            LTB_CHECK(
                this->import_producer( producer, hello.value( ), received.message.handles )
            );
        }
        else if ( MessageType::FrameReady == type )
        {
            LTB_CHECK_VALID( producer.hello.has_value( ), "sent a frame before saying hello" );
            auto const frame = read_message< FrameMessage >( bytes, MessageType::FrameReady );
            LTB_CHECK_VALID( frame.has_value( ), "malformed frame message" );
            LTB_CHECK_VALID( frame->slot < producer.slots.size( ) );

            // Only the newest frame is shown. An older one that was never copied can go
            // straight back to the producer.
            if ( producer.ready_slot.has_value( ) )
            {
                this->send_release( producer, producer.ready_slot.value( ) );
            }
            producer.ready_slot = frame->slot;
        }
        else
        {
            return LTB_MAKE_UNEXPECTED_ERROR( "unexpected message" );
        }
    }

    return utils::success( );
}

auto ShareDisplay::import_producer(
    Producer&                         producer,
    HelloMessage const&               hello,
    std::vector< ipc::NativeHandle >& handles
) -> utils::Result< void >
{
    LTB_CHECK( check_hello( hello, handles.size( ), gpu_.physical_device( ).identity( ) ) );

    auto const free_tile = std::ranges::find( tile_in_use_, false );
    LTB_CHECK_VALID( tile_in_use_.end( ) != free_tile, "the display is full" );

    auto const extent = hello.image.settings.extent;

    for ( auto& handle : handles )
    {
        auto& slot = producer.slots.emplace_back( gpu_.device( ) );
        LTB_CHECK( slot.import( hello.image, std::move( handle ) ) );
    }

    LTB_CHECK( producer.texture.initialize( {
        .image = {
            .extent = { extent.width, extent.height, 1U },
            .format = shared_image_format,
            .usage  = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        },
        .memory_properties = vk::MemoryPropertyFlagBits::eDeviceLocal,
    } ) );
    LTB_CHECK( producer.texture_view.initialize( {
        .image  = producer.texture.image( ).get( ),
        .format = shared_image_format,
    } ) );

    auto const  tile = static_cast< uint32 >( std::distance( tile_in_use_.begin( ), free_tile ) );
    auto const& tile_sets = pipeline_.descriptor_sets( ).front( ).get( );
    LTB_CHECK_VALID( tile < tile_sets.size( ) );

    auto const image_info = vk::DescriptorImageInfo{ }
                                .setSampler( sampler_.get( ) )
                                .setImageView( producer.texture_view.get( ) )
                                .setImageLayout( vk::ImageLayout::eShaderReadOnlyOptimal );
    gpu_.device( ).get( ).updateDescriptorSets(
        vk::WriteDescriptorSet{ }
            .setDstSet( tile_sets[ tile ] )
            .setDstBinding( 0U )
            .setDescriptorType( vk::DescriptorType::eCombinedImageSampler )
            .setImageInfo( image_info ),
        { }
    );

    *free_tile     = true;
    producer.tile  = tile;
    producer.hello = hello;

    spdlog::info(
        "Producer {} (process {}) connected: {}x{}",
        producer.id,
        hello.process_id,
        hello.image.settings.extent.width,
        hello.image.settings.extent.height
    );
    return utils::success( );
}

auto ShareDisplay::remove_disconnected_producers( ) -> utils::Result< void >
{
    if ( std::ranges::none_of( producers_, &Producer::disconnected ) )
    {
        return utils::success( );
    }

    // Frames in flight may still be sampling these producers' textures.
    VK_CHECK( gpu_.device( ).get( ).waitIdle( ) );

    for ( auto const& producer : producers_ )
    {
        if ( producer.disconnected && producer.tile.has_value( ) )
        {
            tile_in_use_.at( producer.tile.value( ) ) = false;
        }
    }
    std::erase_if( producers_, []( Producer const& producer ) -> bool {
        return producer.disconnected;
    } );

    return utils::success( );
}

auto ShareDisplay::send_release( Producer const& producer, uint32 const slot ) -> void
{
    if ( producer.disconnected )
    {
        return;
    }
    auto const sent = producer.channel.send_value(
        FrameMessage{
            .type = MessageType::SlotReleased,
            .slot = slot,
        }
    );
    if ( !sent )
    {
        spdlog::debug( "Couldn't release slot {} of producer {}", slot, producer.id );
    }
}

auto ShareDisplay::send_releases( uint32 const frame_index ) -> void
{
    auto& releases = releases_by_frame_.at( frame_index );
    for ( auto const& release : releases )
    {
        auto const producer = std::ranges::find( producers_, release.producer_id, &Producer::id );
        if ( producers_.end( ) != producer )
        {
            this->send_release( *producer, release.slot );
        }
    }
    releases.clear( );
}

auto ShareDisplay::render( ) -> utils::Result< void >
{
    LTB_CHECK( auto const maybe_frame, cmd_and_sync_.start_frame( presentation_.swapchain( ) ) );
    if ( !maybe_frame.has_value( ) )
    {
        // The swapchain is out of date.
        return this->rebuild_presentation( vlk::objs::Rebuild::Always );
    }
    auto const& frame = maybe_frame.value( );

    // `start_frame` waited for this frame slot's previous work, so the producer images it
    // copied from can be handed back.
    this->send_releases( frame.frame_index );

    VK_CHECK( frame.command_buffer.begin( vk::CommandBufferBeginInfo{ } ) );

    this->record_copies( frame );

    LTB_CHECK( presentation_.begin_render_pass( {
        .command_buffer    = frame.command_buffer,
        .image_index       = frame.image_index,
        .color_clear_value = { 0.08F, 0.08F, 0.1F, 1.0F },
    } ) );
    this->record_tiles( frame );
    frame.command_buffer.endRenderPass( );

    VK_CHECK( frame.command_buffer.end( ) );

    LTB_CHECK( cmd_and_sync_.end_frame( frame, graphics_queue_ ) );
    LTB_CHECK(
        auto const swapchain_status,
        cmd_and_sync_.present_frame( frame, presentation_.swapchain( ), present_queue_ )
    );
    cmd_and_sync_.increment_frame( );

    if ( vlk::objs::SwapchainStatus::NeedsRebuild == swapchain_status )
    {
        return this->rebuild_presentation( vlk::objs::Rebuild::Always );
    }
    return utils::success( );
}

auto ShareDisplay::record_copies( vlk::objs::FrameInfo const& frame ) -> void
{
    auto const& commands = frame.command_buffer;

    for ( auto& producer : producers_ )
    {
        if ( !producer.ready_slot.has_value( ) || producer.disconnected )
        {
            continue;
        }
        auto const  slot_index = producer.ready_slot.value( );
        auto const& shared     = producer.slots[ slot_index ].image( );
        auto const& texture    = producer.texture.image( );

        // Take ownership of the producer's image (matching the producer's release).
        auto acquire = image_barrier(
            shared.get( ),
            shared_image_layout,
            vk::ImageLayout::eTransferSrcOptimal
        );
        acquire.srcQueueFamilyIndex = vk::QueueFamilyExternal;
        acquire.dstQueueFamilyIndex = graphics_family_;
        acquire.setDstAccessMask( vk::AccessFlagBits::eTransferRead );
        commands.pipelineBarrier(
            vk::PipelineStageFlagBits::eTopOfPipe,
            vk::PipelineStageFlagBits::eTransfer,
            { },
            { },
            { },
            acquire
        );

        // Earlier frames may still be sampling the texture, so wait for their fragment
        // shaders before overwriting it.
        commands.pipelineBarrier(
            vk::PipelineStageFlagBits::eFragmentShader,
            vk::PipelineStageFlagBits::eTransfer,
            { },
            { },
            { },
            image_barrier(
                texture.get( ),
                producer.texture_layout,
                vk::ImageLayout::eTransferDstOptimal
            )
                .setDstAccessMask( vk::AccessFlagBits::eTransferWrite )
        );

        auto const& extent = texture.settings( ).extent;
        auto const  layers = vk::ImageSubresourceLayers{ }
                                .setAspectMask( vk::ImageAspectFlagBits::eColor )
                                .setLayerCount( 1U );
        commands.copyImage(
            shared.get( ),
            vk::ImageLayout::eTransferSrcOptimal,
            texture.get( ),
            vk::ImageLayout::eTransferDstOptimal,
            vk::ImageCopy{ }.setSrcSubresource( layers ).setDstSubresource( layers ).setExtent(
                extent
            )
        );

        // Make the copy visible to this frame's fragment shader.
        commands.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eFragmentShader,
            { },
            { },
            { },
            image_barrier(
                texture.get( ),
                vk::ImageLayout::eTransferDstOptimal,
                vk::ImageLayout::eShaderReadOnlyOptimal
            )
                .setSrcAccessMask( vk::AccessFlagBits::eTransferWrite )
                .setDstAccessMask( vk::AccessFlagBits::eShaderRead )
        );

        // Give the image back to the external queue family in the layout the producer
        // hands it over in, so both processes agree on its state.
        auto release = image_barrier(
            shared.get( ),
            vk::ImageLayout::eTransferSrcOptimal,
            shared_image_layout
        );
        release.srcQueueFamilyIndex = graphics_family_;
        release.dstQueueFamilyIndex = vk::QueueFamilyExternal;
        commands.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eBottomOfPipe,
            { },
            { },
            { },
            release
        );

        producer.texture_layout = vk::ImageLayout::eShaderReadOnlyOptimal;
        producer.has_frame      = true;
        producer.ready_slot.reset( );
        releases_by_frame_.at( frame.frame_index )
            .push_back( { .producer_id = producer.id, .slot = slot_index } );
    }
}

auto ShareDisplay::record_tiles( vlk::objs::FrameInfo const& frame ) -> void
{
    auto const visible = std::ranges::count_if( producers_, []( Producer const& producer ) -> bool {
        return producer.has_frame && !producer.disconnected;
    } );
    if ( 0 == visible )
    {
        return;
    }

    pipeline_.bind( frame.command_buffer );

    // Lay the tiles out in a grid of square-ish cells that fills the window.
    auto const window_extent = presentation_.swapchain( ).settings( ).extent;
    auto const window_size   = glm::vec2( window_extent.width, window_extent.height );
    auto const columns
        = static_cast< int32 >( std::ceil( std::sqrt( static_cast< float32 >( visible ) ) ) );
    auto const rows      = static_cast< int32 >( ( visible + columns - 1 ) / columns );
    auto const cell_size = window_size / glm::vec2( columns, rows );

    auto const& tile_sets = pipeline_.descriptor_sets( ).front( ).get( );

    auto index = 0;
    for ( auto const& producer : producers_ )
    {
        if ( !producer.has_frame || producer.disconnected || !producer.tile.has_value( ) )
        {
            continue;
        }

        // Fit the producer's image in its cell, keeping its aspect ratio.
        constexpr auto margin     = 0.94F;
        auto const&    extent     = producer.hello->image.settings.extent;
        auto const     image_size = glm::vec2( extent.width, extent.height );
        auto const     fit = std::min( cell_size.x / image_size.x, cell_size.y / image_size.y );
        auto const     tile_size = image_size * fit * margin;

        auto const cell        = glm::vec2( index % columns, index / columns );
        auto const tile_center = ( cell + 0.5F ) * cell_size;

        auto const params = TileParams{
            .offset = ( tile_center / window_size ) * 2.0F - 1.0F,
            .scale  = tile_size / window_size,
        };

        frame.command_buffer.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics,
            pipeline_.pipeline_layout( ).get( ),
            0U,
            tile_sets[ producer.tile.value( ) ],
            { }
        );
        frame.command_buffer.pushConstants(
            pipeline_.pipeline_layout( ).get( ),
            vk::ShaderStageFlagBits::eVertex,
            0U,
            sizeof( params ),
            &params
        );

        constexpr auto quad_vertices = 6U;
        frame.command_buffer.draw( quad_vertices, 1U, 0U, 0U );
        ++index;
    }
}

auto ShareDisplay::rebuild_presentation( vlk::objs::Rebuild const rebuild ) -> utils::Result< void >
{
    return presentation_.rebuild( { .rebuild = rebuild } );
}

} // namespace ltb::share
