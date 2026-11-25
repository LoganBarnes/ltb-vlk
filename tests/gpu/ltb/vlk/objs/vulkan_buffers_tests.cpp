// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/vlk/objs/vulkan_buffers.hpp"

// project
#include "ltb/vlk/headless_gpu_fixture.hpp"

namespace ltb::vlk
{
namespace
{

using VulkanBuffersTest = HeadlessGpuTest;

TEST_F( VulkanBuffersTest, FailedInitializeCanBeRetried )
{
    auto buffers = objs::VulkanBuffers{ gpu_ };

    auto const valid = BufferSettings{
        .size  = 64U,
        .usage = vk::BufferUsageFlagBits::eUniformBuffer,
    };
    // Concurrent sharing without queue families is rejected before reaching Vulkan.
    auto const invalid = BufferSettings{
        .size         = 64U,
        .usage        = vk::BufferUsageFlagBits::eUniformBuffer,
        .sharing_mode = vk::SharingMode::eConcurrent,
    };
    constexpr auto memory_properties
        = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;

    EXPECT_FALSE( buffers
                      .initialize( {
                          .buffers           = { valid, invalid },
                          .memory_properties = memory_properties,
                      } )
                      .has_value( ) );
    EXPECT_FALSE( buffers.is_initialized( ) );

    auto const result = buffers.initialize( {
        .buffers           = { valid, valid },
        .memory_properties = memory_properties,
    } );
    ASSERT_TRUE( result.has_value( ) ) << result.error( ).error_message( );
    EXPECT_EQ( buffers.layout( ).ranges.size( ), 2U );
}

} // namespace
} // namespace ltb::vlk
