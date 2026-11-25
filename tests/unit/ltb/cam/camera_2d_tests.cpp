// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/cam/camera_2d.hpp"

// project
#include "ltb/cam/camera_render_params.hpp"

// external
#include <glm/gtc/matrix_access.hpp>
#include <gtest/gtest.h>

// standard
#include <cmath>

namespace ltb::cam
{
namespace
{

auto all_finite( glm::mat4 const& matrix ) -> bool
{
    for ( auto col = 0; col < 4; ++col )
    {
        for ( auto row = 0; row < 4; ++row )
        {
            if ( !std::isfinite( matrix[ col ][ row ] ) )
            {
                return false;
            }
        }
    }
    return true;
}

TEST( Camera2dTests, ResizeToEmptyViewportIsIgnored )
{
    auto camera = Camera2d{ };
    camera.resize( glm::vec2( 800.0F, 600.0F ) );
    auto const before = camera.render_params( );

    // A minimized window reports a 0x0 framebuffer.
    camera.resize( glm::vec2( 0.0F, 0.0F ) );
    auto const after = camera.render_params( );

    EXPECT_TRUE( all_finite( after.clip_from_world ) );
    EXPECT_TRUE( all_finite( after.world_from_clip ) );
    EXPECT_EQ( after.clip_from_world, before.clip_from_world );
}

} // namespace
} // namespace ltb::cam
