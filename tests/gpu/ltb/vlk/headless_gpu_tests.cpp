// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/vlk/headless_gpu_fixture.hpp"

namespace ltb::vlk
{
namespace
{

TEST_F( HeadlessGpuTest, InitializesWithoutSurface )
{
    EXPECT_TRUE( gpu_.is_initialized( ) );
    EXPECT_FALSE( gpu_.has_surface( ) );
    EXPECT_TRUE( gpu_.device( ).is_initialized( ) );
}

} // namespace
} // namespace ltb::vlk
