// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/geom/range.hpp"

// external
#include <glm/glm.hpp>
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

TEST( RangeTests, CenterOfIntegerRange )
{
    EXPECT_EQ( center( Range< int >{ .min = 1, .max = 3 } ), 2 );
    EXPECT_EQ( center( Range< int >{ .min = 1, .max = 1 } ), 1 );
    EXPECT_EQ( center( Range< int >{ .min = -3, .max = 5 } ), 1 );
}

TEST( RangeTests, CenterOfVectorRanges )
{
    EXPECT_EQ(
        center( Range2i{ .min = glm::ivec2( 1, 0 ), .max = glm::ivec2( 3, 4 ) } ),
        glm::ivec2( 2, 2 )
    );
    EXPECT_EQ(
        center( Range2{ .min = glm::vec2( -1.0F ), .max = glm::vec2( 3.0F ) } ),
        glm::vec2( 1.0F )
    );
}

} // namespace
} // namespace ltb::geom
