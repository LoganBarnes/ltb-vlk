// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/vlk/fence.hpp"

// project
#include "ltb/vlk/headless_gpu_fixture.hpp"

namespace ltb::vlk
{
namespace
{

using FenceTest = HeadlessGpuTest;

TEST_F( FenceTest, DefaultFenceStartsSignaled )
{
    auto fence = Fence{ gpu_.device( ) };
    ASSERT_TRUE( fence.initialize( ).has_value( ) );

    EXPECT_EQ( gpu_.device( ).get( ).getFenceStatus( fence.get( ) ), vk::Result::eSuccess );
}

TEST_F( FenceTest, FenceCreatedWithoutFlagsStartsUnsignaled )
{
    auto fence = Fence{ gpu_.device( ) };
    ASSERT_TRUE( fence.initialize( FenceSettings{ .flags = { } } ).has_value( ) );

    EXPECT_EQ( gpu_.device( ).get( ).getFenceStatus( fence.get( ) ), vk::Result::eNotReady );
}

} // namespace
} // namespace ltb::vlk
