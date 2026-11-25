// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/utils/enum_flags.hpp"

// external
#include <gtest/gtest.h>

// standard
#include <vector>

namespace ltb::utils
{
namespace
{

enum class Color
{
    Red,
    Green,
    Blue,
};

TEST( EnumFlagsTests, HasFlag )
{
    auto const flags = make_flags( Color::Red, Color::Blue );

    EXPECT_TRUE( has_flag( flags, Color::Red ) );
    EXPECT_FALSE( has_flag( flags, Color::Green ) );
    EXPECT_TRUE( has_flag( flags, Color::Blue ) );
    EXPECT_FALSE( has_flag( no_flags< Color >( ), Color::Red ) );
}

TEST( EnumFlagsTests, ToContainerListsSetFlagsInOrder )
{
    auto const flags = make_flags( Color::Blue, Color::Red );

    EXPECT_EQ( to< std::vector >( flags ), ( std::vector{ Color::Red, Color::Blue } ) );
    EXPECT_TRUE( to< std::vector >( no_flags< Color >( ) ).empty( ) );
}

} // namespace
} // namespace ltb::utils
