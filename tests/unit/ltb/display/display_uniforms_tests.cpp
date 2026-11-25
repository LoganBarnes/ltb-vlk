// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/display/display_uniforms.hpp"

// external
#include <gtest/gtest.h>

namespace ltb::display
{
namespace
{

TEST( DisplayUniformsTests, DefaultsMatchShaderConstants )
{
    auto const uniforms = DisplayUniforms{ };

    // See res/shaders/utils/frag_constants_and_uniforms.glsl
    EXPECT_EQ( static_cast< uint32 >( uniforms.color_mode ), 5U );   // color_mode_global_color
    EXPECT_EQ( static_cast< uint32 >( uniforms.shading_mode ), 0U ); // shading_mode_flat
}

} // namespace
} // namespace ltb::display
