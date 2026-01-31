// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/vlk/dd/simple_mesh_2.hpp"

// project
#include "ltb/vlk/buffer_utils.hpp"
#include "ltb/vlk/check.hpp"
#include "ltb/vlk/command_buffer.hpp"
#include "ltb/vlk/device.hpp"
#include "ltb/vlk/device_memory_utils.hpp"

// external
#include <spdlog/spdlog.h>

namespace ltb::vlk::dd
{
//
// auto initialize_mesh(
//     VulkanLinePipeline2dMesh& vulkan_mesh,
//     SimpleMesh2 const&           line_mesh,
//     vk::CommandBuffer const&  command_buffer
// ) -> utils::Result< void >
// {
//     if ( vulkan_mesh.initialized )
//     {
//         return utils::success( );
//     }
//     LTB_CHECK_VALID( vulkan_mesh.device.is_initialized( ) );
//
//     auto const vertex_buffer_size = line_mesh.positions.size( ) * sizeof(
//     SimpleMesh2::PositionType ); LTB_CHECK( vulkan_mesh.vertex_buffer.initialize( {
//         .size  = vertex_buffer_size,
//         .usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
//     } ) );
//
//     LTB_CHECK(
//         auto const vertex_memory_layout,
//         initialize_buffers_memory(
//             vulkan_mesh.vertex_buffer_memory,
//             vulkan_mesh.device,
//             { vulkan_mesh.vertex_buffer },
//             vk::MemoryPropertyFlagBits::eDeviceLocal
//         )
//     );
//
//     auto staging = BufferAndMemory{ vulkan_mesh.device };
//     LTB_CHECK(
//         auto const staging_memory_range,
//         initialize_buffer_and_memory(
//             staging,
//             vulkan_mesh.device,
//             vertex_buffer_size,
//             vk::BufferUsageFlagBits::eTransferSrc,
//             vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
//         )
//     );
//
//     LTB_CHECK_VALID( vertex_buffer_size <= staging_memory_range.size );
//     LTB_CHECK( auto* device_data, staging.memory.map( staging_memory_range, { } ) );
//     LTB_CHECK_VALID(
//         std::memcpy( device_data, line_mesh.positions.data( ), vertex_buffer_size ) ==
//         device_data
//     );
//     staging.memory.unmap( );
//
//     LTB_CHECK_VALID( vulkan_mesh.device.queues( ).contains( QueueType::Graphics ) );
//     auto const& graphics_queue = vulkan_mesh.device.queues( ).at( QueueType::Graphics );
//
//     LTB_CHECK( copy_buffer(
//         command_buffer,
//         graphics_queue,
//         staging.buffer,
//         vulkan_mesh.vertex_buffer,
//         { vk::BufferCopy{ }.setSize( vertex_buffer_size ) }
//     ) );
//
//     vulkan_mesh.vertex_count = static_cast< uint32 >( line_mesh.positions.size( ) );
//
//     vulkan_mesh.initialized = true;
//
//     return utils::success( );
// }
//
// auto initialize_and_append_mesh(
//     Device&                                device,
//     std::list< VulkanLinePipeline2dMesh >& meshes,
//     SimpleMesh2 const&                        mesh,
//     vk::CommandBuffer const&               command_buffer
// ) -> utils::Result< VulkanLinePipeline2dMesh* >
// {
//     if ( auto const init_result
//          = initialize_mesh( meshes.emplace_back( device ), mesh, command_buffer ) )
//     {
//         return &meshes.back( );
//     }
//     else
//     {
//         meshes.pop_back( );
//         return tl::make_unexpected( init_result.error( ) );
//     }
// }

} // namespace ltb::vlk::dd
