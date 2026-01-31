// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "app_pipeline.hpp"

// project
#include "ltb/vlk/ltb_vlk_config.hpp"
#include "ltb/vlk/check.hpp"
#include "ltb/vlk/device_memory_utils.hpp"
#include "ltb/vlk/objs/frame_info.hpp"

// external
#include "ltb/vlk/command_pool.hpp"
#include <range/v3/range/conversion.hpp>
#include <spdlog/spdlog.h>

namespace ltb
{

ObjsAppPipeline::ObjsAppPipeline(
    vlk::objs::VulkanGpu&          gpu,
    vlk::objs::VulkanPresentation& presentation
)
    : gpu_( gpu )
    , presentation_( presentation )
{
}

auto ObjsAppPipeline::initialize( ObjsAppPipelineSettings settings ) -> utils::Result< void >
{
    if ( this->is_initialized( ) )
    {
        return utils::success( );
    }
    auto shader_modules = std::vector< vlk::ShaderModuleSettings >{
        {
            .spirv_file = vlk::config::shader_dir_path( ) / "tutorial.vert.spv",
            .stage      = vk::ShaderStageFlagBits::eVertex,
        },
        {
            .spirv_file = vlk::config::shader_dir_path( ) / "tutorial.frag.spv",
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
            .setSize( sizeof( MeshPushConstants ) ),
    };

    auto vertex_bindings = std::vector{
        vk::VertexInputBindingDescription{ }
            .setBinding( 0U )
            .setStride( position_size_bytes )
            .setInputRate( vk::VertexInputRate::eVertex ),
        vk::VertexInputBindingDescription{ }
            .setBinding( 1U )
            .setStride( color_size_bytes )
            .setInputRate( vk::VertexInputRate::eVertex ),
    };

    auto vertex_attributes = std::vector{
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

    LTB_CHECK( pipeline_.initialize( {
        .shader_modules         = std::move( shader_modules ),
        .descriptor_set_count   = settings.frame_count,
        .uniform_binding_sets   = { std::move( uniform_bindings ) },
        .uniform_push_constants = std::move( uniform_push_constants ),

        .pipeline = {
            .vertex_bindings    = std::move( vertex_bindings ),
            .vertex_attributes  = std::move( vertex_attributes ),
            .primitive_topology = vk::PrimitiveTopology::eTriangleList,
        },
    } ) );
    LTB_CHECK_VALID( 1UZ == pipeline_.descriptor_sets( ).size( ) );

    auto const& descriptor_sets = pipeline_.descriptor_sets( ).front( ).get( );
    auto const  frame_count     = settings.frame_count;
    for ( auto frame_index = 0U; frame_index < frame_count; ++frame_index )
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

auto ObjsAppPipeline::is_initialized( ) const -> bool
{
    return initialized_;
}

auto ObjsAppPipeline::initialize_mesh(
    TriangleMesh const& mesh,
    vlk::CommandPool&   command_pool,
    vk::Queue const&    queue
) -> utils::Result< MeshPushConstants* >
{
    LTB_CHECK_VALID( command_pool.is_initialized( ) );
    LTB_CHECK_VALID( !mesh.positions.empty( ) );
    LTB_CHECK_VALID( mesh.positions.size( ) == mesh.colors.size( ) );
    LTB_CHECK_VALID( !mesh.indices.empty( ) );

    auto vbo_layout = vlk::MemoryLayout{ };
    vlk::append_memory_size( vbo_layout, mesh.positions.size( ) * position_size_bytes );
    vlk::append_memory_size( vbo_layout, mesh.colors.size( ) * color_size_bytes );
    vlk::append_memory_size( vbo_layout, mesh.indices.size( ) * index_size_bytes );

    auto& mesh_data = mesh_data_.emplace_back( gpu_ );

    LTB_CHECK( mesh_data.vbo.initialize( {
        .layout       = std::move( vbo_layout ),
        .buffer_usage = vk::BufferUsageFlagBits::eVertexBuffer
                      | vk::BufferUsageFlagBits::eIndexBuffer
                      | vk::BufferUsageFlagBits::eTransferDst,
        .memory_properties  = vk::MemoryPropertyFlagBits::eDeviceLocal,
        .store_mapped_value = false,
    } ) );

    auto staging_vbo = vlk::objs::VulkanBuffer{ gpu_ };

    LTB_CHECK( staging_vbo.initialize( {
        .layout       = mesh_data.vbo.layout( ),
        .buffer_usage = vk::BufferUsageFlagBits::eTransferSrc,
        .memory_properties
        = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        .store_mapped_value = false,
    } ) );

    auto const host_data = std::array< void const*, 3U >{
        mesh.positions.data( ),
        mesh.colors.data( ),
        mesh.indices.data( ),
    };
    LTB_CHECK_VALID( host_data.size( ) == staging_vbo.layout( ).ranges.size( ) );
    constexpr auto copy_count = host_data.size( );

    LTB_CHECK(
        auto* const mapped_staging_data,
        staging_vbo.memory( ).map( {
            .size = staging_vbo.layout( ).total_size,
        } )
    );

    for ( auto i = 0UZ; i < copy_count; ++i )
    {
        auto const* const src_data = host_data[ i ];
        auto const&       range    = staging_vbo.layout( ).ranges[ i ];
        auto* const       dst_data = mapped_staging_data + range.offset;

        LTB_CHECK_VALID( std::memcpy( dst_data, src_data, range.size ) == dst_data );
    }
    staging_vbo.memory( ).unmap( );

    auto tmp_command_buffer = vlk::CommandBuffer{ gpu_.device( ), command_pool };
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

    mesh_data.index_count = static_cast< uint32 >( mesh.indices.size( ) );

    return &mesh_data.push_constants;
}

auto ObjsAppPipeline::draw_meshes( vlk::objs::FrameInfo const& frame ) -> utils::Result< void >
{
    for ( auto const& mesh_data : mesh_data_ )
    {
        LTB_CHECK( draw_mesh( mesh_data, frame ) );
    }
    return utils::success( );
}

auto ObjsAppPipeline::pipeline( ) const -> vlk::objs::VulkanGraphicsPipeline const&
{
    return pipeline_;
}

auto ObjsAppPipeline::pipeline( ) -> vlk::objs::VulkanGraphicsPipeline&
{
    return pipeline_;
}

auto ObjsAppPipeline::draw_mesh(
    MeshAndUniforms const&      mesh_data,
    vlk::objs::FrameInfo const& frame
) -> utils::Result< void >
{
    if ( mesh_data.index_count <= 0U )
    {
        return utils::success( );
    }

    constexpr auto model_offset = 0U;
    frame.command_buffer.pushConstants(
        pipeline_.pipeline_layout( ).get( ),
        vk::ShaderStageFlagBits::eVertex,
        model_offset,
        sizeof( mesh_data.push_constants ),
        &mesh_data.push_constants
    );

    auto const& vbo_ranges = mesh_data.vbo.layout( ).ranges;
    LTB_CHECK_VALID( vbo_ranges.size( ) == 3U );

    constexpr auto first_binding  = 0U;
    auto const     vertex_buffers = std::array{
        mesh_data.vbo.buffer( ).get( ),
        mesh_data.vbo.buffer( ).get( ),
    };
    auto const vertex_offsets = std::array{
        vbo_ranges[ 0 ].offset,
        vbo_ranges[ 1 ].offset,
    };
    frame.command_buffer.bindVertexBuffers( first_binding, vertex_buffers, vertex_offsets );

    auto const index_offset = mesh_data.vbo.layout( ).ranges.back( ).offset;
    auto const index_buffer = mesh_data.vbo.buffer( ).get( );
    frame.command_buffer.bindIndexBuffer( index_buffer, index_offset, index_type );

    constexpr auto instance_count = 1U;
    constexpr auto first_index    = 0U;
    constexpr auto vertex_offset  = 0;
    constexpr auto first_instance = 0U;
    frame.command_buffer.drawIndexed(
        mesh_data.index_count,
        instance_count,
        first_index,
        vertex_offset,
        first_instance
    );

    return utils::success( );
}

} // namespace ltb
