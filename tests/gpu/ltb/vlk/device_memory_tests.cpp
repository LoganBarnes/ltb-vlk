// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/vlk/device_memory.hpp"

// project
#include "ltb/vlk/headless_gpu_fixture.hpp"

// standard
#include <vector>

namespace ltb::vlk
{
namespace
{

class DeviceMemoryTest : public HeadlessGpuTest
{
protected:
    auto host_visible_memory( ) -> DeviceMemory
    {
        auto const memory_type_index = gpu_.physical_device( ).find_memory_type_index(
            { vk::MemoryRequirements{ 256U, 0U, ~0U } },
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        );
        EXPECT_TRUE( memory_type_index.has_value( ) );

        auto memory = DeviceMemory{ gpu_.device( ) };
        EXPECT_TRUE( memory
                         .initialize( {
                             .allocation_size   = 256U,
                             .memory_type_index = memory_type_index.value_or( 0U ),
                         } )
                         .has_value( ) );
        return memory;
    }
};

TEST_F( DeviceMemoryTest, UnmapIsSafeToCallRepeatedly )
{
    auto memory = host_visible_memory( );

    memory.unmap( ); // Not mapped yet
    ASSERT_TRUE( memory.map( ).has_value( ) );
    memory.unmap( );
    memory.unmap( ); // Already unmapped

    // Can be mapped again after unmapping.
    EXPECT_TRUE( memory.map( ).has_value( ) );
}

TEST_F( DeviceMemoryTest, MoveTransfersOwnershipAndMappedState )
{
    auto memory = host_visible_memory( );
    ASSERT_TRUE( memory.map( ).has_value( ) );

    auto moved = DeviceMemory{ std::move( memory ) };

    EXPECT_FALSE( memory.is_initialized( ) ); // NOLINT(bugprone-use-after-move)
    EXPECT_TRUE( moved.is_initialized( ) );
    EXPECT_EQ( moved.size( ), 256U );

    // Still mapped, so mapping again is an error until it's unmapped.
    EXPECT_FALSE( moved.map( ).has_value( ) );
    moved.unmap( );
    EXPECT_TRUE( moved.map( ).has_value( ) );
}

TEST_F( DeviceMemoryTest, CanBeStoredInAVector )
{
    auto memories = std::vector< DeviceMemory >{ };
    memories.push_back( host_visible_memory( ) );
    memories.push_back( host_visible_memory( ) );

    EXPECT_TRUE( memories[ 0 ].is_initialized( ) );
    EXPECT_TRUE( memories[ 1 ].is_initialized( ) );
}

} // namespace
} // namespace ltb::vlk
