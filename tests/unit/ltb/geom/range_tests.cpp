// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/geom/range.hpp"

// external
#include <gtest/gtest.h>

namespace ltb::geom
{
namespace
{

TEST( RangeTests, DimensionsIsMaxMinusMin )
{
    constexpr auto range = Range< float >{ .min = -1.0F, .max = 3.0F };
    EXPECT_FLOAT_EQ( dimensions( range ), 4.0F );
}

TEST( RangeTests, CenterOfFloatRange )
{
    constexpr auto range = Range< float >{ .min = -1.0F, .max = 3.0F };
    EXPECT_FLOAT_EQ( center( range ), 1.0F );
}

} // namespace
} // namespace ltb::geom
