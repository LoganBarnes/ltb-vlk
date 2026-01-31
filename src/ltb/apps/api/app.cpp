// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "app.hpp"

// project
#include "ltb/exec/app_defaults.hpp"
#include "ltb/vlk/buffer_utils.hpp"
#include "ltb/vlk/check.hpp"
#include "ltb/vlk/device_memory_utils.hpp"
#include "ltb/vlk/ltb_vlk_config.hpp"

// external
#include "ltb/window/glfw_context.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <range/v3/range/conversion.hpp>
#include <spdlog/spdlog.h>

namespace ltb
{
namespace
{

using VertexPosition = glm::vec3;
using VertexColor    = glm::vec3;
using VertexIndex    = uint32;

constexpr auto vertex_positions = std::array{
    VertexPosition{ -0.5F, -0.5F, +0.0F },
    VertexPosition{ +0.5F, -0.5F, +0.0F },
    VertexPosition{ +0.5F, +0.5F, +0.0F },
    VertexPosition{ -0.5F, +0.5F, +0.0F },

    VertexPosition{ -0.5F, -0.5F, -0.5F },
    VertexPosition{ +0.5F, -0.5F, -0.5F },
    VertexPosition{ +0.5F, +0.5F, -0.5F },
    VertexPosition{ -0.5F, +0.5F, -0.5F },
};
constexpr auto vertex_positions_size = static_cast< uint32 >( sizeof( vertex_positions ) );

constexpr auto vertex_colors = std::array{
    VertexColor{ 1.0F, 0.0F, 0.0F },
    VertexColor{ 0.0F, 1.0F, 0.0F },
    VertexColor{ 0.0F, 0.0F, 1.0F },
    VertexColor{ 1.0F, 1.0F, 1.0F },

    VertexColor{ 1.0F, 0.0F, 0.0F },
    VertexColor{ 0.0F, 1.0F, 0.0F },
    VertexColor{ 0.0F, 0.0F, 1.0F },
    VertexColor{ 1.0F, 1.0F, 1.0F },
};
constexpr auto vertex_colors_size = static_cast< uint32 >( sizeof( vertex_colors ) );

constexpr auto indices = std::array{
    VertexIndex{ 0U },
    VertexIndex{ 1U },
    VertexIndex{ 2U },
    VertexIndex{ 2U },
    VertexIndex{ 3U },
    VertexIndex{ 0U },

    VertexIndex{ 4U },
    VertexIndex{ 5U },
    VertexIndex{ 6U },
    VertexIndex{ 6U },
    VertexIndex{ 7U },
    VertexIndex{ 4U },
};
constexpr auto index_buffer_size = static_cast< uint32 >( sizeof( indices ) );

struct CameraBufferObject
{
    glm::mat4 view_from_world = glm::identity< glm::mat4 >( );
    glm::mat4 clip_from_view  = glm::identity< glm::mat4 >( );
    glm::mat4 clip_from_world = glm::identity< glm::mat4 >( );
};

struct ModelBufferObject
{
    glm::mat4 transform = glm::identity< glm::mat4 >( );
};

auto initialize_buffer_and_memory(
    vlk::Buffer&                  buffer,
    vlk::DeviceMemory&            memory,
    vlk::Device const&            device,
    vk::DeviceSize const          size,
    vk::BufferUsageFlags const    buffer_usage,
    vk::MemoryPropertyFlags const memory_properties
) -> utils::Result< vk::DeviceSize >
{
    LTB_CHECK( buffer.initialize( {
        .size  = size,
        .usage = buffer_usage,
    } ) );

    auto const  memory_requirements = buffer.memory_requirements( );
    auto const& physical_device     = device.physical_device( );

    LTB_CHECK(
        auto const vertex_memory_type_index,
        physical_device.find_memory_type_index( { memory_requirements }, memory_properties )
    );

    LTB_CHECK( memory.initialize( {
        .allocation_size   = memory_requirements.size,
        .memory_type_index = vertex_memory_type_index,
    } ) );

    constexpr auto memory_offset = 0U;
    VK_CHECK( device.get( ).bindBufferMemory( buffer.get( ), memory.get( ), memory_offset ) );

    return memory_requirements.size;
}

auto update_model_uniforms( utils::Duration const& duration_since_start )
{
    auto const& time_seconds = utils::to_seconds< float32 >( duration_since_start );

    auto const     rotation_angle = time_seconds * glm::radians( 90.0F );
    constexpr auto rotation_axis  = glm::vec3( 0.0F, 0.0F, 1.0F );

    return ModelBufferObject{
        .transform = glm::rotate( glm::identity< glm::mat4 >( ), rotation_angle, rotation_axis ),
    };
}

auto update_camera_uniforms( vk::Extent2D const& extent )
{
    constexpr auto eye    = glm::vec3( 2.0F, 2.0F, 2.0F );
    constexpr auto center = glm::vec3( 0.0F, 0.0F, 0.0F );
    constexpr auto up     = glm::vec3( 0.0F, 0.0F, 1.0F );

    auto const view_from_model = glm::lookAt( eye, center, up );

    auto const aspect_ratio
        = static_cast< float32 >( extent.width ) / static_cast< float32 >( extent.height );
    constexpr auto fov_y      = glm::radians( 45.0F );
    constexpr auto near_plane = 0.1F;
    constexpr auto far_plane  = 10.0F;

    auto clip_from_view = glm::perspective( fov_y, aspect_ratio, near_plane, far_plane );

    // OpenGL -> Vulkan coordinate system adjustment
    clip_from_view[ 1 ][ 1 ] *= -1;

    return CameraBufferObject{
        .view_from_world = view_from_model,
        .clip_from_view  = clip_from_view,
        .clip_from_world = clip_from_view * view_from_model,
    };
}

} // namespace

ApiApp::ApiApp( window::GlfwContext& glfw_context, window::GlfwWindow& glfw_window )
    : glfw_context_( glfw_context )
    , glfw_window_( glfw_window )
{
}

auto ApiApp::initialize( ) -> utils::Result< exec::UpdateLoopStatus >
{
    if ( this->is_initialized( ) )
    {
        return exec::UpdateLoopStatus{ };
    }
    LTB_CHECK_VALID( glfw_context_.is_initialized( ) );
    LTB_CHECK_VALID( glfw_window_.is_initialized( ) );

    LTB_CHECK( this->initialize_vulkan( )
                   .and_then( &ApiApp::initialize_swapchain )
                   .and_then( &ApiApp::initialize_graphics )
                   .and_then( &ApiApp::initialize_command_and_sync )
                   .and_then( &ApiApp::initialize_camera_buffers )
                   .and_then( &ApiApp::set_camera_buffers )
                   .and_then( &ApiApp::initialize_vertex_buffer ) );

    timer_.start( );

    LTB_CHECK_VALID( gpu_.graphics_queue );
    LTB_CHECK_VALID( gpu_.present_queue );

    initialized_ = true;

    return exec::UpdateLoopStatus{ };
}

auto ApiApp::is_initialized( ) const -> bool
{
    return initialized_;
}

auto ApiApp::fixed_step_update( exec::UpdateLoopStatus const& status ) -> exec::UpdateRequests
{
    return status.requests;
}

auto ApiApp::frame_update( exec::UpdateLoopStatus const& status ) -> exec::UpdateRequests
{
    if ( auto result = this->render( ); !result )
    {
        spdlog::error( "ApiApp::render() failed: {}", result.error( ).debug_error_message( ) );
    }

    return status.requests;
}

auto ApiApp::on_resize( glm::ivec2 const size ) -> utils::Result< void >
{
    utils::ignore( size );

    VK_CHECK( gpu_.device.get( ).waitIdle( ) );

    spdlog::debug( "rebuild_swapchain()" );

    LTB_CHECK( this->initialize_swapchain( ).and_then( &ApiApp::set_camera_buffers ) );

    return utils::success( );
}

auto ApiApp::clean_up( ) -> utils::Result< void >
{
    VK_CHECK( gpu_.device.get( ).waitIdle( ) );

    return utils::success( );
}

auto ApiApp::initialize_vulkan( ) -> utils::Result< ApiApp* >
{
    LTB_CHECK_VALID( glfw_context_.is_initialized( ) );
    LTB_CHECK_VALID( glfw_window_.is_initialized( ) );

    LTB_CHECK( gpu_.instance.initialize( { }, &glfw_context_ ) );
    LTB_CHECK( gpu_.surface.initialize( ) );
    LTB_CHECK( gpu_.physical_device.initialize( { }, &gpu_.surface ) );
    LTB_CHECK( gpu_.device.initialize( ) );

    LTB_CHECK_VALID( gpu_.device.queues( ).contains( vlk::QueueType::Graphics ) );
    gpu_.graphics_queue = gpu_.device.queues( ).at( vlk::QueueType::Graphics );

    LTB_CHECK_VALID( gpu_.device.queues( ).contains( vlk::QueueType::Surface ) );
    gpu_.present_queue = gpu_.device.queues( ).at( vlk::QueueType::Surface );

    LTB_CHECK( gpu_.descriptor_pool.initialize( { } ) );

    return this;
}

auto ApiApp::initialize_swapchain( ) -> utils::Result< ApiApp* >
{
    LTB_CHECK_VALID( gpu_.instance.is_initialized( ) );
    LTB_CHECK_VALID( gpu_.surface.is_initialized( ) );
    LTB_CHECK_VALID( gpu_.physical_device.is_initialized( ) );
    LTB_CHECK_VALID( gpu_.device.is_initialized( ) );

    present_.depth_image_view.reset( );
    present_.depth_image_memory.reset( );
    present_.depth_image.reset( );
    present_.framebuffers.clear( );
    present_.swapchain_image_views.clear( );

    auto const window_framebuffer_size = glfw_window_.framebuffer_size( );

    auto const extent = vk::Extent2D{
        static_cast< uint32 >( window_framebuffer_size.x ),
        static_cast< uint32 >( window_framebuffer_size.y ),
    };

    auto swapchain_settings = vlk::SwapchainSettings{ .extent = extent };

    LTB_CHECK(
        present_.swapchain.initialize( std::move( swapchain_settings ), vlk::Reinitialize::Always )
    );

    present_.swapchain_image_views.reserve( present_.swapchain.images( ).size( ) );
    for ( auto const& image : present_.swapchain.images( ) )
    {
        LTB_CHECK( present_.swapchain_image_views.emplace_back( gpu_.device )
                       .initialize( {
                           .image  = image,
                           .format = present_.swapchain.image_format( ),
                       } ) );
    }
    LTB_CHECK_VALID(
        present_.swapchain.images( ).size( ) == present_.swapchain_image_views.size( )
    );

    LTB_CHECK( present_.depth_image.initialize( {
        .extent = { extent.width, extent.height, 1 },
        .format = gpu_.physical_device.depth_image_format( ),
        .usage  = vk::ImageUsageFlagBits::eDepthStencilAttachment,
    } ) );

    auto const& depth_memory_requirements = present_.depth_image.memory_requirements( );

    LTB_CHECK(
        auto const depth_memory_type_index,
        gpu_.physical_device.find_memory_type_index(
            { depth_memory_requirements },
            vk::MemoryPropertyFlagBits::eDeviceLocal
        )
    );

    LTB_CHECK( present_.depth_image_memory.initialize( {
        .allocation_size   = depth_memory_requirements.size,
        .memory_type_index = depth_memory_type_index,
    } ) );

    constexpr auto memory_offset = 0U;
    VK_CHECK( gpu_.device.get( ).bindImageMemory(
        present_.depth_image.get( ),
        present_.depth_image_memory.get( ),
        memory_offset
    ) );

    LTB_CHECK( present_.depth_image_view.initialize( {
        .image       = present_.depth_image.get( ),
        .format      = present_.depth_image.settings( ).format,
        .aspect_mask = vk::ImageAspectFlagBits::eDepth,
    } ) );

    if ( !present_.render_pass.is_initialized( ) )
    {
        LTB_CHECK( present_.render_pass.initialize( {
            .color_attachment_format = present_.swapchain.image_format( ),
            .depth_attachment_format = present_.depth_image.settings( ).format,
        } ) );
    }

    present_.framebuffers.reserve( present_.swapchain_image_views.size( ) );
    for ( auto const& image_view : present_.swapchain_image_views )
    {
        LTB_CHECK( present_.framebuffers.emplace_back( gpu_.device, present_.render_pass )
                       .initialize( {
                           .attachments = { image_view.get( ), present_.depth_image_view.get( ) },
                           .extent      = present_.swapchain.settings( ).extent,
                           .layers      = 1U,
                       } ) );
    }
    LTB_CHECK_VALID( present_.swapchain_image_views.size( ) == present_.framebuffers.size( ) );

    return this;
}

auto ApiApp::initialize_graphics( ) -> utils::Result< ApiApp* >
{
    LTB_CHECK_VALID( gpu_.device.is_initialized( ) );
    LTB_CHECK_VALID( gpu_.descriptor_pool.is_initialized( ) );
    LTB_CHECK_VALID( present_.render_pass.is_initialized( ) );

    LTB_CHECK( graphics_.shader_modules.emplace_back( gpu_.device )
                   .initialize( {
                       .spirv_file = vlk::config::shader_dir_path( ) / "tutorial.vert.spv",
                       .stage      = vk::ShaderStageFlagBits::eVertex,
                   } ) );
    LTB_CHECK( graphics_.shader_modules.emplace_back( gpu_.device )
                   .initialize( {
                       .spirv_file = vlk::config::shader_dir_path( ) / "tutorial.frag.spv",
                       .stage      = vk::ShaderStageFlagBits::eFragment,
                   } ) );

    LTB_CHECK( graphics_.descriptor_set_layout.initialize( {
        .bindings = {
            vk::DescriptorSetLayoutBinding{ }
                .setBinding( 0U )
                .setDescriptorType( vk::DescriptorType::eUniformBuffer )
                .setDescriptorCount( 1U )
                .setStageFlags( vk::ShaderStageFlagBits::eVertex ),
        },
    } ) );

    LTB_CHECK( graphics_.descriptor_sets.initialize( {
        .layouts = {
            exec::max_frames_in_flight,
            graphics_.descriptor_set_layout.get( ),
        },
    } ) );

    LTB_CHECK( graphics_.pipeline_layout.initialize( {
        .descriptor_set_layouts = { graphics_.descriptor_set_layout.get( ) },
        .push_constant_ranges   = {
            vk::PushConstantRange{ }
                .setStageFlags( vk::ShaderStageFlagBits::eVertex )
                .setOffset( 0U )
                .setSize( sizeof( ModelBufferObject ) ),
        },
    } ) );

    static_assert( std::is_same_v< VertexPosition, glm::vec3 > );
    constexpr auto position_size   = sizeof( VertexPosition );
    constexpr auto position_format = vk::Format::eR32G32B32Sfloat;
    static_assert( std::is_same_v< VertexColor, glm::vec3 > );
    constexpr auto color_size   = sizeof( VertexColor );
    constexpr auto color_format = vk::Format::eR32G32B32Sfloat;

    auto const vertex_bindings = std::vector{
        vk::VertexInputBindingDescription{ }
            .setBinding( 0U )
            .setStride( position_size )
            .setInputRate( vk::VertexInputRate::eVertex ),
        vk::VertexInputBindingDescription{ }
            .setBinding( 1U )
            .setStride( color_size )
            .setInputRate( vk::VertexInputRate::eVertex ),
    };

    auto const vertex_attributes = std::vector{
        vk::VertexInputAttributeDescription{ }
            .setBinding( 0U )
            .setLocation( 0U )
            .setFormat( position_format )
            .setOffset( 0U ),
        vk::VertexInputAttributeDescription{ }
            .setBinding( 1U )
            .setLocation( 1U )
            .setFormat( color_format )
            .setOffset( 0U ),
    };

    LTB_CHECK( graphics_.pipeline.initialize( {
        .vertex_bindings   = vertex_bindings,
        .vertex_attributes = vertex_attributes,
    } ) );

    return this;
}

auto ApiApp::initialize_command_and_sync( ) -> utils::Result< ApiApp* >
{
    LTB_CHECK_VALID( gpu_.device.is_initialized( ) );
    LTB_CHECK_VALID( gpu_.physical_device.is_initialized( ) );
    LTB_CHECK_VALID( present_.swapchain.is_initialized( ) );

    LTB_CHECK( cmd_and_sync_.command_pool.initialize( {
        .queue_type = vlk::QueueType::Graphics,
    } ) );

    for ( auto frame_index = 0U; frame_index < exec::max_frames_in_flight; ++frame_index )
    {
        LTB_CHECK( cmd_and_sync_.command_buffers
                       .emplace_back( gpu_.device, cmd_and_sync_.command_pool )
                       .initialize( ) );
        LTB_CHECK( cmd_and_sync_.in_flight_fences.emplace_back( gpu_.device ).initialize( ) );
        LTB_CHECK(
            cmd_and_sync_.image_available_semaphores.emplace_back( gpu_.device ).initialize( )
        );
    }

    auto const image_count = static_cast< uint32 >( present_.swapchain.images( ).size( ) );
    for ( auto image_index = 0UL; image_index < image_count; ++image_index )
    {
        LTB_CHECK(
            cmd_and_sync_.render_finished_semaphores.emplace_back( gpu_.device ).initialize( )
        );
    }

    return this;
}

auto ApiApp::initialize_camera_buffers( ) -> utils::Result< ApiApp* >
{
    camera_uniform_.layout.ranges.reserve( exec::max_frames_in_flight );
    for ( auto i = 0U; i < exec::max_frames_in_flight; ++i )
    {
        camera_uniform_.layout.ranges.push_back( {
            .size   = sizeof( CameraBufferObject ),
            .offset = camera_uniform_.layout.total_size,
        } );
        camera_uniform_.layout.total_size += camera_uniform_.layout.ranges.back( ).size;
    }

    LTB_CHECK( camera_uniform_.buffer.initialize( {
        .size  = camera_uniform_.layout.total_size,
        .usage = vk::BufferUsageFlagBits::eUniformBuffer,
    } ) );

    auto const memory_requirements = camera_uniform_.buffer.memory_requirements( );

    LTB_CHECK(
        auto const vertex_memory_type_index,
        gpu_.physical_device.find_memory_type_index(
            { memory_requirements },
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        )
    );

    LTB_CHECK( camera_uniform_.memory.initialize( {
        .allocation_size   = memory_requirements.size,
        .memory_type_index = vertex_memory_type_index,
    } ) );

    constexpr auto memory_offset = 0U;
    VK_CHECK( gpu_.device.get( ).bindBufferMemory(
        camera_uniform_.buffer.get( ),
        camera_uniform_.memory.get( ),
        memory_offset
    ) );

    LTB_CHECK(
        camera_uniform_.mapped_data,
        camera_uniform_.memory.map( { .size = memory_requirements.size }, { } )
    );

    auto const& descriptor_sets = graphics_.descriptor_sets.get( );
    for ( auto frame_index = 0U; frame_index < exec::max_frames_in_flight; ++frame_index )
    {
        LTB_CHECK_VALID( frame_index < camera_uniform_.layout.ranges.size( ) );
        LTB_CHECK_VALID( frame_index < descriptor_sets.size( ) );

        auto const& memory_range   = camera_uniform_.layout.ranges[ frame_index ];
        auto const& descriptor_set = descriptor_sets[ frame_index ];

        auto const descriptor_buffer_info = vk::DescriptorBufferInfo{ }
                                                .setBuffer( camera_uniform_.buffer.get( ) )
                                                .setOffset( memory_range.offset )
                                                .setRange( memory_range.size );

        auto const descriptor_writes = std::vector{
            vk::WriteDescriptorSet{ }
                .setDstSet( descriptor_set )
                .setDstBinding( 0U )
                .setDstArrayElement( 0U )
                .setDescriptorType( vk::DescriptorType::eUniformBuffer )
                .setBufferInfo( descriptor_buffer_info ),
        };

        gpu_.device.get( ).updateDescriptorSets( descriptor_writes, { } );
    }

    return this;
}

auto ApiApp::set_camera_buffers( ) -> utils::Result< ApiApp* >
{
    auto const ubo = update_camera_uniforms( present_.swapchain.settings( ).extent );

    for ( auto frame_index = 0U; frame_index < exec::max_frames_in_flight; ++frame_index )
    {
        LTB_CHECK_VALID( frame_index < camera_uniform_.layout.ranges.size( ) );
        auto const& memory_range = camera_uniform_.layout.ranges[ frame_index ];

        auto* dst_data = camera_uniform_.mapped_data + memory_range.offset;
        LTB_CHECK_VALID( memory_range.size >= sizeof( ubo ) );
        LTB_CHECK_VALID( std::memcpy( dst_data, &ubo, sizeof( ubo ) ) == dst_data );
    }

    return this;
}

auto ApiApp::initialize_vertex_buffer( ) -> utils::Result< ApiApp* >
{
    LTB_CHECK_VALID( gpu_.device.is_initialized( ) );
    LTB_CHECK_VALID( gpu_.graphics_queue );

    vbo_.position_range = {
        .size   = vertex_positions_size,
        .offset = 0U,
    };
    vbo_.color_range = {
        .size   = vertex_colors_size,
        .offset = vbo_.position_range.size,
    };
    vbo_.index_range = {
        .size   = index_buffer_size,
        .offset = vbo_.position_range.size + vbo_.color_range.size,
    };
    auto const total_buffer_size
        = vbo_.position_range.size + vbo_.color_range.size + vbo_.index_range.size;

    LTB_CHECK(
        auto const vbo_memory_size,
        initialize_buffer_and_memory(
            vbo_.buffer,
            vbo_.memory,
            gpu_.device,
            total_buffer_size,
            vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer
                | vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        )
    );
    LTB_CHECK_VALID( total_buffer_size <= vbo_memory_size );

    auto staging_buffer = vlk::Buffer{ gpu_.device };
    auto staging_memory = vlk::DeviceMemory{ gpu_.device };

    LTB_CHECK(
        auto const staging_memory_size,
        initialize_buffer_and_memory(
            staging_buffer,
            staging_memory,
            gpu_.device,
            total_buffer_size,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        )
    );
    LTB_CHECK_VALID( total_buffer_size <= staging_memory_size );

    LTB_CHECK( auto* data, staging_memory.map( { .size = staging_memory_size }, { } ) );

    constexpr auto host_data = std::array< void const*, 3U >{
        vertex_positions.data( ),
        vertex_colors.data( ),
        indices.data( ),
    };
    auto const host_ranges = std::array{
        vbo_.position_range,
        vbo_.color_range,
        vbo_.index_range,
    };
    for ( auto i = 0UZ; i < 3UZ; ++i )
    {
        auto const* src_data = host_data.at( i );
        auto const& range    = host_ranges.at( i );
        auto*       dst_data = data + range.offset;

        LTB_CHECK_VALID( std::memcpy( dst_data, src_data, range.size ) == dst_data );
    }
    staging_memory.unmap( );

    // Copy from staging buffer to vertex buffer
    auto tmp_command_buffer = vlk::CommandBuffer{ gpu_.device, cmd_and_sync_.command_pool };
    LTB_CHECK( tmp_command_buffer.initialize( ) );

    VK_CHECK( tmp_command_buffer.get( ).begin(
        vk::CommandBufferBeginInfo{ }.setFlags( vk::CommandBufferUsageFlagBits::eOneTimeSubmit )
    ) );

    tmp_command_buffer.get( ).copyBuffer(
        staging_buffer.get( ),
        vbo_.buffer.get( ),
        { vk::BufferCopy{ }.setSize( total_buffer_size ) }
    );

    VK_CHECK( tmp_command_buffer.get( ).end( ) );

    auto const submit_info = vk::SubmitInfo{ }.setCommandBuffers( tmp_command_buffer.get( ) );

    VK_CHECK( gpu_.graphics_queue.submit( submit_info, nullptr ) );
    VK_CHECK( gpu_.graphics_queue.waitIdle( ) );

    return this;
}

auto ApiApp::render( ) -> utils::Result< void >
{
    constexpr auto max_possible_timeout = std::numeric_limits< uint32 >::max( );

    auto const& command_buffer
        = cmd_and_sync_.command_buffers[ cmd_and_sync_.current_frame_index ].get( );
    auto const& in_flight_fence
        = cmd_and_sync_.in_flight_fences[ cmd_and_sync_.current_frame_index ].get( );
    auto const& image_available_semaphore
        = cmd_and_sync_.image_available_semaphores[ cmd_and_sync_.current_frame_index ].get( );

    auto const     fences       = std::array{ in_flight_fence };
    constexpr auto wait_for_all = true;
    VK_CHECK( gpu_.device.get( ).waitForFences( fences, wait_for_all, max_possible_timeout ) );

    auto const image_index_result = gpu_.device.get( ).acquireNextImageKHR(
        present_.swapchain.get( ),
        max_possible_timeout,
        image_available_semaphore
    );
    if ( image_index_result.result == vk::Result::eErrorOutOfDateKHR )
    {
        // Swapchain needs to be recreated. Not a real error.
        return utils::success( );
    }
    VK_CHECK( auto const image_index, image_index_result );

    VK_CHECK( gpu_.device.get( ).resetFences( fences ) );

    constexpr auto reset_flags = vk::CommandBufferResetFlags{ };
    VK_CHECK( command_buffer.reset( reset_flags ) );

    LTB_CHECK( this->record_render_commands( {
        .command_buffer = command_buffer,
        .image_index    = image_index,
        .frame_index    = cmd_and_sync_.current_frame_index,
    } ) );

    auto const& render_finished_semaphore
        = cmd_and_sync_.render_finished_semaphores[ image_index ].get( );

    auto const     wait_semaphores = std::array{ image_available_semaphore };
    constexpr auto wait_stages     = std::array< vk::PipelineStageFlags, 1U >{
        vk::PipelineStageFlagBits::eColorAttachmentOutput
    };
    auto const signal_semaphores = std::array{ render_finished_semaphore };

    auto const submit_info = vk::SubmitInfo{ }
                                 .setWaitSemaphores( wait_semaphores )
                                 .setWaitDstStageMask( wait_stages )
                                 .setCommandBuffers( command_buffer )
                                 .setSignalSemaphores( signal_semaphores );

    VK_CHECK( gpu_.graphics_queue.submit( submit_info, in_flight_fence ) );

    auto const present_info = vk::PresentInfoKHR{ }
                                  .setWaitSemaphores( signal_semaphores )
                                  .setSwapchains( present_.swapchain.get( ) )
                                  .setImageIndices( image_index );

    switch ( auto const present_result = gpu_.present_queue.presentKHR( present_info ) )
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

    cmd_and_sync_.current_frame_index
        = ( cmd_and_sync_.current_frame_index + 1U ) % exec::max_frames_in_flight;

    return utils::success( );
}

auto ApiApp::record_render_commands( FrameInfo const& frame ) -> utils::Result< void >
{
    VK_CHECK( frame.command_buffer.begin( vk::CommandBufferBeginInfo{ } ) );

    LTB_CHECK_VALID( frame.image_index < present_.framebuffers.size( ) );
    auto& framebuffer = present_.framebuffers[ frame.image_index ];

    auto const extent = present_.swapchain.settings( ).extent;

    auto const clear_values = std::array{
        vk::ClearValue{ }.setColor( { 0.35F, 0.35F, 0.35F, 1.0F } ),
        vk::ClearValue{ }.setDepthStencil( { 1.0F, 0U } ),
    };
    auto const render_pass_info
        = vk::RenderPassBeginInfo{ }
              .setRenderPass( present_.render_pass.get( ) )
              .setFramebuffer( framebuffer.get( ) )
              .setRenderArea( vk::Rect2D{ }.setOffset( { 0, 0 } ).setExtent( extent ) )
              .setClearValues( clear_values );

    frame.command_buffer.beginRenderPass( render_pass_info, vk::SubpassContents::eInline );

    auto const viewports = std::vector{
        vk::Viewport{ }
            .setX( 0.0F )
            .setY( 0.0F )
            .setWidth( static_cast< float32 >( extent.width ) )
            .setHeight( static_cast< float32 >( extent.height ) )
            .setMinDepth( 0.0F )
            .setMaxDepth( 1.0F ),
    };
    constexpr auto first_viewport_index = 0UL;
    frame.command_buffer.setViewport( first_viewport_index, viewports );

    auto const scissors = std::vector{
        vk::Rect2D{ }.setOffset( { 0, 0 } ).setExtent( present_.swapchain.settings( ).extent ),
    };
    constexpr auto first_scissor_index = 0UL;
    frame.command_buffer.setScissor( first_scissor_index, scissors );

    frame.command_buffer
        .bindPipeline( vk::PipelineBindPoint::eGraphics, graphics_.pipeline.get( ) );

    auto const descriptor_sets = graphics_.descriptor_sets.get( );
    LTB_CHECK_VALID( frame.frame_index < descriptor_sets.size( ) );

    constexpr auto bind_point = vk::PipelineBindPoint::eGraphics;
    constexpr auto first_set  = 0U;
    frame.command_buffer.bindDescriptorSets(
        bind_point,
        graphics_.pipeline_layout.get( ),
        first_set,
        descriptor_sets[ frame.frame_index ],
        { }
    );

    auto const model_uniform = update_model_uniforms( timer_.duration_since_start( ) );

    constexpr auto model_offset = 0U;
    frame.command_buffer.pushConstants(
        graphics_.pipeline_layout.get( ),
        vk::ShaderStageFlagBits::eVertex,
        model_offset,
        sizeof( model_uniform ),
        &model_uniform
    );

    constexpr auto first_binding  = 0U;
    auto const     vertex_buffers = std::array{
        vbo_.buffer.get( ),
        vbo_.buffer.get( ),
    };
    auto const vertex_offsets = std::array{
        vbo_.position_range.offset,
        vbo_.color_range.offset,
    };
    frame.command_buffer.bindVertexBuffers( first_binding, vertex_buffers, vertex_offsets );

    static_assert( std::is_same_v< VertexIndex, uint32 > );
    constexpr auto index_type = vk::IndexType::eUint32;

    auto const index_offset = vbo_.index_range.offset;
    auto const index_buffer = vbo_.buffer.get( );
    frame.command_buffer.bindIndexBuffer( index_buffer, index_offset, index_type );

    constexpr auto index_count    = static_cast< uint32 >( indices.size( ) );
    constexpr auto instance_count = 1U;
    constexpr auto first_index    = 0U;
    constexpr auto vertex_offset  = 0;
    constexpr auto first_instance = 0U;
    frame.command_buffer
        .drawIndexed( index_count, instance_count, first_index, vertex_offset, first_instance );

    frame.command_buffer.endRenderPass( );

    VK_CHECK( frame.command_buffer.end( ) );

    return utils::success( );
}

} // namespace ltb
