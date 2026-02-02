// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/vlk/dd/lines_pipeline_2.hpp"

// project
#include "ltb/vlk/check.hpp"
#include "ltb/vlk/device_memory_utils.hpp"
#include "ltb/vlk/ltb_vlk_config.hpp"
#include "ltb/vlk/objs/frame_info.hpp"

// external
#include "ltb/vlk/command_pool.hpp"
#include <range/v3/range/conversion.hpp>
#include <spdlog/spdlog.h>

namespace ltb::vlk::dd
{

LinesPipeline2::LinesPipeline2( objs::VulkanGpu& gpu, objs::VulkanPresentation& presentation )
    : gpu_( gpu )
    , presentation_( presentation )
{
}

auto LinesPipeline2::initialize( LinesPipeline2Settings const& settings ) -> utils::Result< void >
{
    if ( this->is_initialized( ) )
    {
        return utils::success( );
    }
    LTB_CHECK_VALID( settings.frame_count > 0U );
    LTB_CHECK_VALID( settings.camera_ubo.is_initialized( ) );

    auto shader_modules = std::vector< ShaderModuleSettings >{
        {
            .spirv_file = config::shader_dir_path( ) / "lines_2d.vert.spv",
            .stage      = vk::ShaderStageFlagBits::eVertex,
        },
        {
            .spirv_file = config::shader_dir_path( ) / "lines_2d.frag.spv",
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

    auto uniform_push_constants = std::vector{
        vk::PushConstantRange{ }
            .setStageFlags( vk::ShaderStageFlagBits::eVertex )
            .setOffset( 0U )
            .setSize( sizeof( SimpleModelUniforms ) ),
        vk::PushConstantRange{ }
            .setStageFlags( vk::ShaderStageFlagBits::eFragment )
            .setOffset( sizeof( SimpleModelUniforms ) )
            .setSize( sizeof( SimpleDisplayUniforms ) ),
    };

    auto vertex_bindings = std::vector{
        vk::VertexInputBindingDescription{ }
            .setBinding( 0U )
            .setStride( SimpleMesh2::position_size_bytes )
            .setInputRate( vk::VertexInputRate::eVertex ),
    };

    auto vertex_attributes = std::vector{
        vk::VertexInputAttributeDescription{ }
            .setBinding( 0U )
            .setLocation( 0U )
            .setFormat( SimpleMesh2::position_format )
            .setOffset( 0U ),
    };

    LTB_CHECK( pipeline_.initialize( {
        .shader_modules         = std::move( shader_modules ),
        .descriptor_set_count   = settings.frame_count,
        .uniform_binding_sets   = { std::move( uniform_bindings ) },
        .uniform_push_constants = std::move( uniform_push_constants ),

        .pipeline = {
            .vertex_bindings    = std::move( vertex_bindings ),
            .vertex_attributes  = std::move( vertex_attributes ),
            .primitive_topology = vk::PrimitiveTopology::eLineList,
            .depth_stencil      = std::nullopt,
        },
    } ) );

    auto const& descriptor_sets_list = pipeline_.descriptor_sets( );
    LTB_CHECK_VALID( 1UZ == descriptor_sets_list.size( ) );

    auto const& descriptor_sets = descriptor_sets_list.front( ).get( );
    for ( auto frame_index = 0U; frame_index < settings.frame_count; ++frame_index )
    {
        LTB_CHECK_VALID( frame_index < settings.camera_ubo.layout( ).ranges.size( ) );
        LTB_CHECK_VALID( frame_index < descriptor_sets.size( ) );

        auto const& memory_range   = settings.camera_ubo.layout( ).ranges[ frame_index ];
        auto const& descriptor_set = descriptor_sets[ frame_index ];

        auto const descriptor_buffer_info = vk::DescriptorBufferInfo{ }
                                                .setBuffer( settings.camera_ubo.buffer( ).get( ) )
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

    initialized_ = true;

    return utils::success( );
}

auto LinesPipeline2::is_initialized( ) const -> bool
{
    return initialized_;
}

auto LinesPipeline2::initialize_mesh(
    SimpleMesh2 const& mesh,
    CommandPool&       command_pool,
    vk::Queue const&   queue
) -> utils::Result< SimpleMeshUniforms* >
{
    LTB_CHECK_VALID( command_pool.is_initialized( ) );
    LTB_CHECK_VALID( !mesh.positions.empty( ) );

    auto const vertex_count   = static_cast< uint32 >( mesh.positions.size( ) );
    auto const positions_size = vertex_count * SimpleMesh2::position_size_bytes;

    auto vbo_layout = MemoryLayout{ };
    append_memory_size( vbo_layout, positions_size );

    auto& mesh_data = mesh_data_.emplace_back( gpu_ );

    LTB_CHECK( mesh_data.vbo.initialize( {
        .layout       = std::move( vbo_layout ),
        .buffer_usage = vk::BufferUsageFlagBits::eVertexBuffer
                      | vk::BufferUsageFlagBits::eIndexBuffer
                      | vk::BufferUsageFlagBits::eTransferDst,
        .memory_properties  = vk::MemoryPropertyFlagBits::eDeviceLocal,
        .store_mapped_value = false,
    } ) );
    LTB_CHECK_VALID( positions_size <= mesh_data.vbo.layout( ).total_size );

    auto staging_vbo = objs::VulkanBuffer{ gpu_ };

    LTB_CHECK( staging_vbo.initialize( {
        .layout       = mesh_data.vbo.layout( ),
        .buffer_usage = vk::BufferUsageFlagBits::eTransferSrc,
        .memory_properties
        = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        .store_mapped_value = false,
    } ) );
    LTB_CHECK_VALID( positions_size <= staging_vbo.layout( ).total_size );

    LTB_CHECK(
        auto* const mapped_staging_data,
        staging_vbo.memory( ).map( {
            .size = staging_vbo.layout( ).total_size,
        } )
    );
    LTB_CHECK_VALID(
        std::memcpy( mapped_staging_data, mesh.positions.data( ), positions_size )
        == mapped_staging_data
    );
    staging_vbo.memory( ).unmap( );

    auto tmp_command_buffer = CommandBuffer{ gpu_.device( ), command_pool };
    LTB_CHECK( tmp_command_buffer.initialize( ) );
    auto const& command_buffer = tmp_command_buffer.get( );

    VK_CHECK( command_buffer.begin(
        vk::CommandBufferBeginInfo{ }.setFlags( vk::CommandBufferUsageFlagBits::eOneTimeSubmit )
    ) );

    command_buffer.copyBuffer(
        staging_vbo.buffer( ).get( ),
        mesh_data.vbo.buffer( ).get( ),
        { vk::BufferCopy{ }.setSize( staging_vbo.layout( ).total_size ) }
    );

    VK_CHECK( command_buffer.end( ) );

    auto const submit_info = vk::SubmitInfo{ }.setCommandBuffers( command_buffer );

    VK_CHECK( queue.submit( submit_info, nullptr ) );
    VK_CHECK( queue.waitIdle( ) );

    mesh_data.draw_count = vertex_count;

    return &mesh_data.uniforms;
}

auto LinesPipeline2::draw( objs::FrameInfo const& frame ) -> utils::Result< void >
{
    pipeline_.bind( frame.command_buffer );

    LTB_CHECK( pipeline_.bind_descriptor_sets( frame ) );

    for ( auto const& mesh_data : mesh_data_ )
    {
        LTB_CHECK( draw_mesh( mesh_data, frame ) );
    }
    return utils::success( );
}

auto LinesPipeline2::draw_mesh( MeshAndUniforms const& mesh_data, objs::FrameInfo const& frame )
    -> utils::Result< void >
{
    if ( mesh_data.draw_count <= 0U )
    {
        return utils::success( );
    }

    constexpr auto model_offset = 0U;
    frame.command_buffer.pushConstants(
        pipeline_.pipeline_layout( ).get( ),
        vk::ShaderStageFlagBits::eVertex,
        model_offset,
        sizeof( mesh_data.uniforms.model ),
        &mesh_data.uniforms.model
    );
    frame.command_buffer.pushConstants(
        pipeline_.pipeline_layout( ).get( ),
        vk::ShaderStageFlagBits::eFragment,
        sizeof( mesh_data.uniforms.model ),
        sizeof( mesh_data.uniforms.display ),
        &mesh_data.uniforms.display
    );

    constexpr auto first_binding  = 0U;
    auto const     vertex_buffers = std::array{ mesh_data.vbo.buffer( ).get( ) };
    constexpr auto vertex_offsets = std::array{ vk::DeviceSize{ 0U } };
    frame.command_buffer.bindVertexBuffers( first_binding, vertex_buffers, vertex_offsets );

    constexpr auto instance_count = 1U;
    constexpr auto first_vertex   = 0U;
    constexpr auto first_instance = 0U;
    frame.command_buffer.draw( mesh_data.draw_count, instance_count, first_vertex, first_instance );

    return utils::success( );
}

} // namespace ltb::vlk::dd
