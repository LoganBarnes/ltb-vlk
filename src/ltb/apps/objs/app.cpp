// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "app.hpp"

// project
#include "ltb/exec/app_defaults.hpp"
#include "ltb/vlk/ltb_vlk_config.hpp"
#include "ltb/vlk/buffer_utils.hpp"
#include "ltb/vlk/check.hpp"
#include "ltb/vlk/device_memory_utils.hpp"

// external
#include "ltb/cam/camera_2d.hpp"
#include "ltb/window/glfw_context.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <range/v3/range/conversion.hpp>
#include <spdlog/spdlog.h>

namespace ltb
{
namespace
{

auto update_model_uniforms( utils::Duration const& duration_since_start, float32 const scale )
{
    auto const& time_seconds = utils::to_seconds< float32 >( duration_since_start );

    auto const     rotation_angle = time_seconds * glm::radians( 90.0F ) * scale;
    constexpr auto rotation_axis  = glm::vec3( 0.0F, 0.0F, 1.0F );

    return ObjsAppPipeline::MeshPushConstants{
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

    return cam::CameraRenderParams{
        .view_from_world = view_from_model,
        .clip_from_view  = clip_from_view,
        .clip_from_world = clip_from_view * view_from_model,
    };
}

} // namespace

ObjsApp::ObjsApp( window::GlfwContext& glfw_context, window::GlfwWindow& glfw_window )
    : glfw_context_( glfw_context )
    , glfw_window_( glfw_window )
{
}

auto ObjsApp::initialize( ) -> utils::Result< exec::UpdateLoopStatus >
{
    if ( this->is_initialized( ) )
    {
        return exec::UpdateLoopStatus{};
    }
    LTB_CHECK_VALID( glfw_context_.is_initialized( ) );
    LTB_CHECK_VALID( glfw_window_.is_initialized( ) );

    LTB_CHECK( this->initialize_gpu_presentation( )
                   .and_then( &ObjsApp::initialize_camera_buffers )
                   .and_then( &ObjsApp::initialize_graphics )
                   .and_then( &ObjsApp::initialize_command_and_sync )
                   .and_then( &ObjsApp::set_camera_buffers )
                   .and_then( &ObjsApp::initialize_vertex_buffer ) );

    LTB_CHECK_VALID( graphics_queue_ );
    LTB_CHECK_VALID( present_queue_ );

    initialized_ = true;

    return exec::UpdateLoopStatus{};
}

auto ObjsApp::is_initialized( ) const -> bool
{
    return initialized_;
}

auto ObjsApp::fixed_step_update( exec::UpdateLoopStatus const& status ) -> exec::UpdateRequests
{
    *top_model_uniforms_    = update_model_uniforms( status.cumulative_time, +1.0F );
    *bottom_model_uniforms_ = update_model_uniforms( status.cumulative_time, -1.0F );

    return status.requests;
}

auto ObjsApp::frame_update( exec::UpdateLoopStatus const& status ) -> exec::UpdateRequests
{
    if ( auto result = this->render( ); !result )
    {
        spdlog::error( "ObjsApp::render() failed: {}", result.error( ).debug_error_message( ) );
    }

    return status.requests;
}

auto ObjsApp::on_resize( glm::ivec2 const size ) -> utils::Result< void >
{
    utils::ignore( size );

    VK_CHECK( gpu_.device( ).get( ).waitIdle( ) );

    LTB_CHECK( presentation_.rebuild( { } ) );
    LTB_CHECK( this->set_camera_buffers( ) );

    return utils::success( );
}

auto ObjsApp::clean_up( ) -> utils::Result< void >
{
    VK_CHECK( gpu_.device( ).get( ).waitIdle( ) );

    return utils::success( );
}

auto ObjsApp::initialize_gpu_presentation( ) -> utils::Result< ObjsApp* >
{
    LTB_CHECK( gpu_.initialize( { } ) );

    LTB_CHECK_VALID( gpu_.device( ).queues( ).contains( vlk::QueueType::Graphics ) );
    graphics_queue_ = gpu_.device( ).queues( ).at( vlk::QueueType::Graphics );

    LTB_CHECK_VALID( gpu_.device( ).queues( ).contains( vlk::QueueType::Surface ) );
    present_queue_ = gpu_.device( ).queues( ).at( vlk::QueueType::Surface );

    LTB_CHECK( presentation_.initialize( { } ) );

    return this;
}

auto ObjsApp::initialize_camera_buffers( ) -> utils::Result< ObjsApp* >
{
    auto camera_buffer_layout = vlk::MemoryLayout{ };
    vlk::append_memory_size_n(
        camera_buffer_layout,
        sizeof( cam::CameraRenderParams ),
        exec::max_frames_in_flight
    );

    LTB_CHECK( camera_ubo_.initialize( {
        .layout       = std::move( camera_buffer_layout ),
        .buffer_usage = vk::BufferUsageFlagBits::eUniformBuffer,
        .memory_properties
        = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        .store_mapped_value = true,
    } ) );

    return this;
}

auto ObjsApp::initialize_graphics( ) -> utils::Result< ObjsApp* >
{
    LTB_CHECK( graphics_.initialize( {
        .frame_count = exec::max_frames_in_flight,
        .camera_ubo  = camera_ubo_,
    } ) );

    return this;
}

auto ObjsApp::initialize_command_and_sync( ) -> utils::Result< ObjsApp* >
{
    LTB_CHECK( cmd_and_sync_.initialize( {
        .frame_count  = exec::max_frames_in_flight,
        .image_count  = static_cast< uint32 >( presentation_.swapchain( ).images( ).size( ) ),
        .command_pool = {
            .queue_type = vlk::QueueType::Graphics,
        },
    } ) );

    return this;
}

auto ObjsApp::set_camera_buffers( ) -> utils::Result< ObjsApp* >
{
    auto const ubo = update_camera_uniforms( presentation_.swapchain( ).settings( ).extent );

    for ( auto frame_index = 0U; frame_index < exec::max_frames_in_flight; ++frame_index )
    {
        LTB_CHECK_VALID( frame_index < camera_ubo_.layout( ).ranges.size( ) );
        auto const& memory_range = camera_ubo_.layout( ).ranges[ frame_index ];

        auto* dst_data = camera_ubo_.mapped_data( ) + memory_range.offset;
        LTB_CHECK_VALID( memory_range.size >= sizeof( ubo ) );
        LTB_CHECK_VALID( std::memcpy( dst_data, &ubo, sizeof( ubo ) ) == dst_data );
    }

    return this;
}

auto ObjsApp::initialize_vertex_buffer( ) -> utils::Result< ObjsApp* >
{
    LTB_CHECK_VALID( graphics_queue_ );

    auto const vertex_colors = std::vector< ObjsAppPipeline::VertexColor >{
        { 1.0F, 0.0F, 0.0F },
        { 0.0F, 1.0F, 0.0F },
        { 0.0F, 0.0F, 1.0F },
        { 1.0F, 1.0F, 1.0F },
    };

    auto const top_mesh = ObjsAppPipeline::TriangleMesh{
        .positions{
            { -0.5F, -0.5F, +0.0F },
            { +0.5F, -0.5F, +0.0F },
            { +0.5F, +0.5F, +0.0F },
            { -0.5F, +0.5F, +0.0F },
        },
        .colors  = vertex_colors,
        .indices = { 0U, 1U, 2U, 2U, 3U, 0U },
    };

    auto const bottom_mesh = ObjsAppPipeline::TriangleMesh{
        .positions = std::vector< ObjsAppPipeline::VertexPosition >{
            { -0.5F, -0.5F, -0.5F },
            { +0.5F, -0.5F, -0.5F },
            { +0.5F, +0.5F, -0.5F },
            { -0.5F, +0.5F, -0.5F },
        },
        .colors =  vertex_colors,
        .indices = { 0U, 1U, 2U, 2U, 3U, 0U },
    };

    LTB_CHECK(
        top_model_uniforms_,
        graphics_.initialize_mesh( top_mesh, cmd_and_sync_.command_pool( ), graphics_queue_ )
    );
    LTB_CHECK(
        bottom_model_uniforms_,
        graphics_.initialize_mesh( bottom_mesh, cmd_and_sync_.command_pool( ), graphics_queue_ )
    );

    return this;
}

auto ObjsApp::render( ) -> utils::Result< void >
{
    LTB_CHECK( auto const maybe_frame, cmd_and_sync_.start_frame( presentation_.swapchain( ) ) );

    if ( maybe_frame.has_value( ) )
    {
        auto const& frame = maybe_frame.value( );

        LTB_CHECK( this->record_render_commands( frame ) );
        LTB_CHECK( cmd_and_sync_.end_frame( frame, graphics_queue_ ) );
        LTB_CHECK(
            cmd_and_sync_.present_frame( frame, presentation_.swapchain( ), present_queue_ )
        );
    }
    return utils::success( );
}

auto ObjsApp::record_render_commands( vlk::objs::FrameInfo const& frame ) -> utils::Result< void >
{
    VK_CHECK( frame.command_buffer.begin( vk::CommandBufferBeginInfo{ } ) );

    LTB_CHECK( presentation_.begin_render_pass( {
        .command_buffer    = frame.command_buffer,
        .image_index       = frame.image_index,
        .color_clear_value = { 0.35F, 0.35F, 0.35F, 1.0F },
    } ) );

    graphics_.pipeline( ).bind( frame.command_buffer );

    LTB_CHECK( graphics_.pipeline( ).bind_descriptor_sets( frame ) );

    LTB_CHECK( graphics_.draw_meshes( frame ) );

    frame.command_buffer.endRenderPass( );

    VK_CHECK( frame.command_buffer.end( ) );

    return utils::success( );
}

} // namespace ltb
