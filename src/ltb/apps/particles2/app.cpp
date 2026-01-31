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
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/transform.hpp>
#include <spdlog/spdlog.h>

// standard
#include <random>

namespace ltb
{
namespace
{

constexpr auto particle_count       = 1'000'001U;
constexpr auto particle_buffer_size = particle_count * sizeof( Particle );

struct ComputeUniforms
{
    float32 delta_time = 0.0F;
};

} // namespace

Particles2App::Particles2App( window::GlfwContext& glfw_context, window::GlfwWindow& glfw_window )
    : glfw_context_( glfw_context )
    , glfw_window_( glfw_window )
{
}

auto Particles2App::initialize( ) -> utils::Result< exec::UpdateLoopStatus >
{
    if ( this->is_initialized( ) )
    {
        return exec::UpdateLoopStatus{ };
    }

    LTB_CHECK( this->initialize_gpu_presentation( )
                   .and_then( &Particles2App::initialize_compute_pipeline )
                   .and_then( &Particles2App::initialize_compute_uniforms )
                   .and_then( &Particles2App::initialize_particles )
                   .and_then( &Particles2App::initialize_display_pipeline )
                   .and_then( &Particles2App::initialize_camera ) );

    camera_.set_width( 10.0F );

    // Pre-record compute command buffers instead of doing it on each frame.
    for ( auto frame_index = 0U; frame_index < exec::max_frames_in_flight; ++frame_index )
    {
        LTB_CHECK(
            auto const frame_objects,
            compute_cmd_and_sync_.get_frame_objects( frame_index )
        );

        constexpr auto reset_flags = vk::CommandBufferResetFlags{ };
        VK_CHECK( frame_objects.command_buffer.reset( reset_flags ) );

        LTB_CHECK( this->record_compute_commands( {
            .command_buffer  = frame_objects.command_buffer,
            .frame_fence     = frame_objects.frame_fence,
            .frame_index     = frame_index,
            .image_semaphore = nullptr,
            .image_index     = 0U,
        } ) );
    }

    initialized_ = true;

    return exec::UpdateLoopStatus{ };
}

auto Particles2App::is_initialized( ) const -> bool
{
    return initialized_;
}

auto Particles2App::fixed_step_update( exec::UpdateLoopStatus const& status )
    -> exec::UpdateRequests
{
    if ( delta_time_ != status.update_time_step )
    {
        delta_time_ = status.update_time_step;
        compute_frames_updated_.clear( );
    }

    if ( auto result = this->compute( ); !result )
    {
        spdlog::error(
            "Particles2App::compute() failed:\n"
            "{}",
            result.error( ).debug_error_message( )
        );
    }

    return status.requests;
}

auto Particles2App::frame_update( exec::UpdateLoopStatus const& status ) -> exec::UpdateRequests
{
    imgui_.new_frame( );
    this->configure_gui( );

    if ( auto result = this->render( ); !result )
    {
        spdlog::error(
            "Particles2App::render() failed:\n"
            "{}",
            result.error( ).debug_error_message( )
        );
    }

    return status.requests;
}

auto Particles2App::configure_gui( ) -> void
{
    if ( camera_.handle_inputs( ) )
    {
        camera_frames_updated_.clear( );
    }

    ImGui::DockSpaceOverViewport( 0, nullptr, ImGuiDockNodeFlags_PassthruCentralNode );

    if ( ImGui::Begin( "Info" ) )
    {
        ImGui::Text( "Particles: %u", particle_count );
        ImGui::Text( "FPS: %.1f", ImGui::GetIO( ).Framerate );
    }
    ImGui::End( );
}

auto Particles2App::on_resize( glm::ivec2 const size ) -> utils::Result< void >
{
    camera_.resize( glm::vec2( size ) );
    camera_frames_updated_.clear( );

    return presentation_.rebuild( {
        .swapchain = presentation_.swapchain( ).settings( ),
    } );
}

auto Particles2App::clean_up( ) -> utils::Result< void >
{
    VK_CHECK( gpu_.device( ).get( ).waitIdle( ) );

    return utils::success( );
}

auto Particles2App::initialize_gpu_presentation( ) -> utils::Result< Particles2App* >
{
    LTB_CHECK( gpu_.initialize( { } ) );

    LTB_CHECK_VALID( gpu_.device( ).queues( ).contains( vlk::QueueType::Graphics ) );
    auto const& graphics_queue = gpu_.device( ).queues( ).at( vlk::QueueType::Graphics );

    LTB_CHECK_VALID( gpu_.device( ).queues( ).contains( vlk::QueueType::Compute ) );
    auto const& compute_queue = gpu_.device( ).queues( ).at( vlk::QueueType::Compute );

    LTB_CHECK_VALID( graphics_queue == compute_queue );
    graphics_and_compute_queue_ = graphics_queue;

    LTB_CHECK_VALID( gpu_.device( ).queues( ).contains( vlk::QueueType::Surface ) );
    present_queue_ = gpu_.device( ).queues( ).at( vlk::QueueType::Surface );

    auto swapchain_settings = vlk::SwapchainSettings{
        .preferred_present_modes = {
            vk::PresentModeKHR::eImmediate,
            vk::PresentModeKHR::eFifoLatestReady,
            vk::PresentModeKHR::eFifo,
        },
    };
    auto render_pass_settings = vlk::render_pass_settings_3d(
        vk::Format::eUndefined,
        gpu_.physical_device( ).depth_image_format( )
    );

    LTB_CHECK( presentation_.initialize( {
        .swapchain   = std::move( swapchain_settings ),
        .render_pass = std::move( render_pass_settings ),
    } ) );

    LTB_CHECK( imgui_.initialize( ) );

    return this;
}

auto Particles2App::initialize_compute_pipeline( ) -> utils::Result< Particles2App* >
{
    auto shader_module = vlk::ShaderModuleSettings{
        .spirv_file = vlk::config::shader_dir_path( ) / "particles2.comp.spv",
        .stage      = vk::ShaderStageFlagBits::eCompute,
    };

    auto uniform_bindings = std::vector{
        vk::DescriptorSetLayoutBinding{ }
            .setBinding( 0U )
            .setDescriptorType( vk::DescriptorType::eStorageBuffer )
            .setDescriptorCount( 1U )
            .setStageFlags( vk::ShaderStageFlagBits::eCompute ),
        vk::DescriptorSetLayoutBinding{ }
            .setBinding( 1U )
            .setDescriptorType( vk::DescriptorType::eStorageBuffer )
            .setDescriptorCount( 1U )
            .setStageFlags( vk::ShaderStageFlagBits::eCompute ),
        vk::DescriptorSetLayoutBinding{ }
            .setBinding( 2U )
            .setDescriptorType( vk::DescriptorType::eUniformBuffer )
            .setDescriptorCount( 1U )
            .setStageFlags( vk::ShaderStageFlagBits::eCompute ),
    };

    LTB_CHECK( compute_.initialize( {
        .shader_module        = std::move( shader_module ),
        .descriptor_set_count = exec::max_frames_in_flight,
        .uniform_bindings     = std::move( uniform_bindings ),
    } ) );

    LTB_CHECK( compute_cmd_and_sync_.initialize( {
        .frame_count  = exec::max_frames_in_flight,
        .image_count  = 0UZ,
        .command_pool = {
            .queue_type = vlk::QueueType::Compute,
        },
    } ) );

    return this;
}

auto Particles2App::initialize_compute_uniforms( ) -> utils::Result< Particles2App* >
{
    auto const ubo_alignment
        = gpu_.physical_device( ).properties( ).limits.minUniformBufferOffsetAlignment;

    auto unform_buffer_layout = vlk::MemoryLayout{ };
    append_memory_size_n(
        unform_buffer_layout,
        { sizeof( ComputeUniforms ), ubo_alignment },
        exec::max_frames_in_flight
    );

    LTB_CHECK( compute_ubo_.initialize( {
        .layout       = std::move( unform_buffer_layout ),
        .buffer_usage = vk::BufferUsageFlagBits::eUniformBuffer,
        .memory_properties
        = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        .store_mapped_value = true,
    } ) );

    LTB_CHECK_VALID( compute_.is_initialized( ) );

    auto const& compute_descriptor_sets = compute_.descriptor_sets( ).get( );
    for ( auto frame_index = 0U; frame_index < exec::max_frames_in_flight; ++frame_index )
    {
        LTB_CHECK_VALID( frame_index < compute_ubo_.layout( ).ranges.size( ) );
        LTB_CHECK_VALID( frame_index < compute_descriptor_sets.size( ) );

        auto const& memory_range   = compute_ubo_.layout( ).ranges[ frame_index ];
        auto const& descriptor_set = compute_descriptor_sets[ frame_index ];

        auto const descriptor_buffer_info = vk::DescriptorBufferInfo{ }
                                                .setBuffer( compute_ubo_.buffer( ).get( ) )
                                                .setOffset( memory_range.offset )
                                                .setRange( memory_range.size );

        auto const descriptor_writes = std::vector{
            vk::WriteDescriptorSet{ }
                .setDstSet( descriptor_set )
                .setDstBinding( 2U )
                .setDstArrayElement( 0U )
                .setDescriptorType( vk::DescriptorType::eUniformBuffer )
                .setBufferInfo( descriptor_buffer_info ),
        };

        gpu_.device( ).get( ).updateDescriptorSets( descriptor_writes, { } );
    }

    return this;
}

auto Particles2App::initialize_particles( ) -> utils::Result< Particles2App* >
{
    LTB_CHECK_VALID( compute_.is_initialized( ) );

    // Initialize particles
    auto const seed      = std::random_device{ }( );
    auto       rand_gen  = std::default_random_engine{ seed };
    auto       rand_dist = std::uniform_real_distribution( 0.0F, 1.0F );

    // Initial particle positions on a circle
    auto cpu_particles = std::vector< Particle >( particle_count );
    for ( auto& particle : cpu_particles )
    {
        auto const radius = std::sqrt( rand_dist( rand_gen ) );
        auto const theta  = rand_dist( rand_gen ) * glm::two_pi< float32 >( );
        auto const phi    = rand_dist( rand_gen ) * glm::pi< float32 >( );
        auto const x      = radius * std::cos( theta ) * std::cos( phi );
        auto const y      = radius * std::sin( theta ) * std::cos( phi );
        auto const z      = radius * std::sin( phi );

        particle.position   = { x, y, z, 1.0F };
        auto const velocity = glm::normalize( glm::vec3( particle.position ) ) * 0.25F;
        particle.velocity   = glm::vec4( velocity, 0.0F );
    }

    LTB_CHECK_VALID(
        particle_buffer_size == ( cpu_particles.size( ) * sizeof( cpu_particles[ 0 ] ) )
    );

    auto gpu_particles_layout = vlk::MemoryLayout{ };
    vlk::append_memory_size_n(
        gpu_particles_layout,
        particle_buffer_size,
        exec::max_frames_in_flight
    );

    LTB_CHECK( gpu_particles_.initialize( {
        .layout       = std::move( gpu_particles_layout ),
        .buffer_usage = vk::BufferUsageFlagBits::eStorageBuffer
                      | vk::BufferUsageFlagBits::eVertexBuffer
                      | vk::BufferUsageFlagBits::eTransferDst,
        .memory_properties  = vk::MemoryPropertyFlagBits::eDeviceLocal,
        .store_mapped_value = false,
    } ) );

    auto staging = vlk::objs::VulkanBuffer{ gpu_ };

    LTB_CHECK( staging.initialize( {
        .layout       = { .total_size = particle_buffer_size },
        .buffer_usage = vk::BufferUsageFlagBits::eTransferSrc,
        .memory_properties
        = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        .store_mapped_value = false,
    } ) );

    LTB_CHECK_VALID( particle_buffer_size <= gpu_particles_.memory( ).size( ) );
    LTB_CHECK( auto* const device_data, staging.memory( ).map( ) );
    LTB_CHECK_VALID(
        std::memcpy( device_data, cpu_particles.data( ), particle_buffer_size ) == device_data
    );
    staging.memory( ).unmap( );

    auto tmp_copy_command_buffer = vlk::CommandBuffer{
        gpu_.device( ),
        compute_cmd_and_sync_.command_pool( ),
    };
    LTB_CHECK( tmp_copy_command_buffer.initialize( ) );

    auto const copy_ranges = gpu_particles_.layout( ).ranges
                           | ranges::views::transform( vlk::MakeCopyRegion{ } )
                           | ranges::to< std::vector >( );

    LTB_CHECK(
        vlk::copy_buffer(
            tmp_copy_command_buffer.get( ),
            graphics_and_compute_queue_,
            staging.buffer( ),
            gpu_particles_.buffer( ),
            copy_ranges
        )
    );

    auto const& compute_descriptor_sets = compute_.descriptor_sets( ).get( );
    for ( auto frame_index = 0U; frame_index < exec::max_frames_in_flight; ++frame_index )
    {
        auto const prev_frame_index
            = ( ( frame_index + exec::max_frames_in_flight ) - 1U ) % exec::max_frames_in_flight;

        LTB_CHECK_VALID( prev_frame_index < gpu_particles_.layout( ).ranges.size( ) );
        LTB_CHECK_VALID( frame_index < gpu_particles_.layout( ).ranges.size( ) );
        LTB_CHECK_VALID( frame_index < compute_descriptor_sets.size( ) );

        auto const& prev_memory_range = gpu_particles_.layout( ).ranges[ prev_frame_index ];
        auto const& curr_memory_range = gpu_particles_.layout( ).ranges[ frame_index ];
        auto const& descriptor_set    = compute_descriptor_sets[ frame_index ];

        auto const ssbo_prev_frame_info = vk::DescriptorBufferInfo{ }
                                              .setBuffer( gpu_particles_.buffer( ).get( ) )
                                              .setOffset( prev_memory_range.offset )
                                              .setRange( prev_memory_range.size );

        auto const ssbo_curr_frame_info = vk::DescriptorBufferInfo{ }
                                              .setBuffer( gpu_particles_.buffer( ).get( ) )
                                              .setOffset( curr_memory_range.offset )
                                              .setRange( curr_memory_range.size );

        auto const descriptor_writes = std::vector{
            vk::WriteDescriptorSet{ }
                .setDstSet( descriptor_set )
                .setDstBinding( 0U )
                .setDstArrayElement( 0U )
                .setDescriptorType( vk::DescriptorType::eStorageBuffer )
                .setBufferInfo( ssbo_prev_frame_info ),
            vk::WriteDescriptorSet{ }
                .setDstSet( descriptor_set )
                .setDstBinding( 1U )
                .setDstArrayElement( 0U )
                .setDescriptorType( vk::DescriptorType::eStorageBuffer )
                .setBufferInfo( ssbo_curr_frame_info ),
        };

        gpu_.device( ).get( ).updateDescriptorSets( descriptor_writes, { } );
    }

    return this;
}

auto Particles2App::initialize_display_pipeline( ) -> utils::Result< Particles2App* >
{
    auto shader_modules = std::vector{
        vlk::ShaderModuleSettings{
            .spirv_file = vlk::config::shader_dir_path( ) / "particles2.vert.spv",
            .stage      = vk::ShaderStageFlagBits::eVertex,
        },
        vlk::ShaderModuleSettings{
            .spirv_file = vlk::config::shader_dir_path( ) / "particles2.frag.spv",
            .stage      = vk::ShaderStageFlagBits::eFragment,
        },
    };

    auto uniform_bindings = std::vector{
        vk::DescriptorSetLayoutBinding{ }
            .setBinding( 0U )
            .setDescriptorType( vk::DescriptorType::eUniformBuffer )
            .setDescriptorCount( 1U )
            .setStageFlags( vk::ShaderStageFlagBits::eVertex ),
    };

    auto vertex_bindings = std::vector{
        vk::VertexInputBindingDescription{ }
            .setBinding( 0U )
            .setStride( sizeof( Particle ) )
            .setInputRate( vk::VertexInputRate::eVertex ),
    };

    auto vertex_attributes = std::vector{
        vk::VertexInputAttributeDescription{ }
            .setBinding( 0U )
            .setLocation( 0U )
            .setFormat( vk::Format::eR32G32B32Sfloat )
            .setOffset( offsetof( Particle, position ) ),
        vk::VertexInputAttributeDescription{ }
            .setBinding( 0U )
            .setLocation( 1U )
            .setFormat( vk::Format::eR32G32B32Sfloat )
            .setOffset( offsetof( Particle, velocity ) ),
    };

    LTB_CHECK( graphics_.initialize( {
        .shader_modules       = std::move( shader_modules ),
        .descriptor_set_count = exec::max_frames_in_flight,
        .uniform_binding_sets = { std::move( uniform_bindings ) },

        .pipeline = {
            .vertex_bindings    = std::move( vertex_bindings ),
            .vertex_attributes  = std::move( vertex_attributes ),
            .primitive_topology = vk::PrimitiveTopology::ePointList,
        },
    } ) );

    LTB_CHECK( graphics_cmd_and_sync_.initialize( {
        .frame_count  = exec::max_frames_in_flight,
        .image_count  = static_cast< uint32 >( presentation_.swapchain( ).images( ).size( ) ),
        .command_pool = {
            .queue_type = vlk::QueueType::Graphics,
        },
    } ) );

    return this;
}

auto Particles2App::initialize_camera( ) -> utils::Result< Particles2App* >
{
    auto camera_buffer_layout = vlk::MemoryLayout{ };

    append_memory_size_n(
        camera_buffer_layout,
        sizeof( cam::SimpleCameraRenderParams ),
        exec::max_frames_in_flight
    );

    LTB_CHECK( camera_ubo_.initialize( {
        .layout       = std::move( camera_buffer_layout ),
        .buffer_usage = vk::BufferUsageFlagBits::eUniformBuffer,
        .memory_properties
        = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        .store_mapped_value = true,
    } ) );

    LTB_CHECK_VALID( graphics_.is_initialized( ) );
    LTB_CHECK_VALID( 1UZ == graphics_.descriptor_sets( ).size( ) );

    auto const& graphics_descriptor_sets = graphics_.descriptor_sets( ).front( ).get( );
    for ( auto frame_index = 0U; frame_index < exec::max_frames_in_flight; ++frame_index )
    {
        LTB_CHECK_VALID( frame_index < camera_ubo_.layout( ).ranges.size( ) );
        LTB_CHECK_VALID( frame_index < graphics_descriptor_sets.size( ) );

        auto const& memory_range   = camera_ubo_.layout( ).ranges[ frame_index ];
        auto const& descriptor_set = graphics_descriptor_sets[ frame_index ];

        auto const descriptor_buffer_info = vk::DescriptorBufferInfo{ }
                                                .setBuffer( camera_ubo_.buffer( ).get( ) )
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

        gpu_.device( ).get( ).updateDescriptorSets( descriptor_writes, { } );
    }

    return this;
}

auto Particles2App::compute( ) -> utils::Result< void >
{
    LTB_CHECK(
        auto const maybe_frame,
        compute_cmd_and_sync_.start_frame( vlk::objs::ResetCommandBuffer::No )
    );

    if ( maybe_frame.has_value( ) )
    {
        auto const& frame = maybe_frame.value( );

        LTB_CHECK( this->update_compute_uniforms( frame ) );

        // Commands are pre-recorded.

        auto wait_until_signaled = std::vector< vlk::objs::SemaphoreAndStage >{ };

        if ( compute_semaphore_.has_value( ) )
        {
            wait_until_signaled.push_back( {
                .semaphore = compute_semaphore_.value( ),
                .stage     = vk::PipelineStageFlagBits::eComputeShader,
            } );
            compute_semaphore_ = std::nullopt;
        }

        auto const& compute_finished_semaphore
            = compute_cmd_and_sync_.get_frame_objects( ).frame_semaphore;

        LTB_CHECK( compute_cmd_and_sync_.end_frame(
            frame,
            std::move( wait_until_signaled ),
            { compute_finished_semaphore },
            graphics_and_compute_queue_
        ) );

        compute_semaphore_ = compute_finished_semaphore;
        compute_cmd_and_sync_.increment_frame( );
    }

    return utils::success( );
}

auto Particles2App::update_compute_uniforms( vlk::objs::FrameInfo const& frame )
    -> utils::Result< void >
{
    if ( compute_frames_updated_.contains( frame.frame_index ) )
    {
        return utils::success( );
    }

    auto const compute_ubo = ComputeUniforms{
        .delta_time = utils::to_seconds< float32 >( delta_time_ ),
    };

    LTB_CHECK_VALID( frame.frame_index < compute_ubo_.layout( ).ranges.size( ) );
    auto const& memory_range = compute_ubo_.layout( ).ranges[ frame.frame_index ];

    auto* const dst_data = compute_ubo_.mapped_data( ) + memory_range.offset;
    LTB_CHECK_VALID( memory_range.size >= sizeof( compute_ubo ) );
    LTB_CHECK_VALID( std::memcpy( dst_data, &compute_ubo, sizeof( compute_ubo ) ) == dst_data );

    compute_frames_updated_.emplace( frame.frame_index );

    return utils::success( );
}

auto Particles2App::record_compute_commands( vlk::objs::FrameInfo const& frame )
    -> utils::Result< void >
{
    VK_CHECK( frame.command_buffer.begin( vk::CommandBufferBeginInfo{ } ) );

    compute_.bind( frame.command_buffer );

    LTB_CHECK( compute_.bind_descriptor_sets( frame ) );

    constexpr auto group_count = glm::uvec3{ ( particle_count / 256U ) + 1U, 1U, 1U };
    frame.command_buffer.dispatch( group_count.x, group_count.y, group_count.z );

    VK_CHECK( frame.command_buffer.end( ) );

    return utils::success( );
}

auto Particles2App::render( ) -> utils::Result< void >
{
    LTB_CHECK(
        auto const maybe_frame,
        graphics_cmd_and_sync_.start_frame( presentation_.swapchain( ) )
    );

    if ( maybe_frame.has_value( ) )
    {
        auto const frame = maybe_frame.value( );

        LTB_CHECK( this->update_camera_uniforms( frame ) );
        LTB_CHECK( this->record_render_commands( frame ) );

        auto wait_until_signaled = std::vector< vlk::objs::SemaphoreAndStage >{ };

        if ( compute_semaphore_ )
        {
            wait_until_signaled.push_back( {
                .semaphore = compute_semaphore_.value( ),
                .stage     = vk::PipelineStageFlagBits::eVertexInput,
            } );
            compute_semaphore_ = std::nullopt;
        }
        wait_until_signaled.push_back( {
            .semaphore = frame.image_semaphore,
            .stage     = vk::PipelineStageFlagBits::eColorAttachmentOutput,
        } );

        LTB_CHECK(
            auto const& render_finished_semaphore,
            graphics_cmd_and_sync_.get_present_semaphore( frame )
        );

        LTB_CHECK( graphics_cmd_and_sync_.end_frame(
            frame,
            std::move( wait_until_signaled ),
            { render_finished_semaphore },
            graphics_and_compute_queue_
        ) );

        LTB_CHECK( graphics_cmd_and_sync_
                       .present_frame( frame, presentation_.swapchain( ), present_queue_ ) );

        graphics_cmd_and_sync_.increment_frame( );
    }

    return utils::success( );
}

auto Particles2App::update_camera_uniforms( vlk::objs::FrameInfo const& frame )
    -> utils::Result< void >
{
    if ( camera_frames_updated_.contains( frame.frame_index ) )
    {
        return utils::success( );
    }

    auto const camera_ubo = camera_.simple_render_params( );

    LTB_CHECK_VALID( frame.frame_index < camera_ubo_.layout( ).ranges.size( ) );
    auto const& memory_range = camera_ubo_.layout( ).ranges[ frame.frame_index ];

    auto* const dst_data = camera_ubo_.mapped_data( ) + memory_range.offset;
    LTB_CHECK_VALID( memory_range.size >= sizeof( camera_ubo ) );
    LTB_CHECK_VALID( std::memcpy( dst_data, &camera_ubo, sizeof( camera_ubo ) ) == dst_data );

    camera_frames_updated_.emplace( frame.frame_index );

    return utils::success( );
}

auto Particles2App::record_render_commands( vlk::objs::FrameInfo const& frame )
    -> utils::Result< void >
{
    VK_CHECK( frame.command_buffer.begin( vk::CommandBufferBeginInfo{ } ) );

    LTB_CHECK( presentation_.begin_render_pass( {
        .command_buffer    = frame.command_buffer,
        .image_index       = frame.image_index,
        .color_clear_value = { 0.35F, 0.35F, 0.35F, 1.0F },
    } ) );

    graphics_.bind( frame.command_buffer );

    LTB_CHECK( graphics_.bind_descriptor_sets( frame ) );

    auto const compute_frame_index = compute_cmd_and_sync_.frame_index( );
    LTB_CHECK_VALID( compute_frame_index < gpu_particles_.layout( ).ranges.size( ) );
    auto const& vbo_offset = gpu_particles_.layout( ).ranges[ compute_frame_index ].offset;

    constexpr auto first_binding  = 0U;
    auto const     vertex_buffers = std::array{ gpu_particles_.buffer( ).get( ) };
    auto const     vertex_offsets = std::array{ vbo_offset };
    frame.command_buffer.bindVertexBuffers( first_binding, vertex_buffers, vertex_offsets );

    constexpr auto instance_count = 1U;
    constexpr auto first_vertex   = 0U;
    constexpr auto first_instance = 0U;
    frame.command_buffer.draw( particle_count, instance_count, first_vertex, first_instance );

    imgui_.render( frame.command_buffer );

    frame.command_buffer.endRenderPass( );

    VK_CHECK( frame.command_buffer.end( ) );

    return utils::success( );
}

} // namespace ltb
