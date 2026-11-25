// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/vlk/objs/vulkan_command_and_sync.hpp"

// external
#include <gtest/gtest.h>

namespace ltb::vlk::objs
{
namespace
{

TEST( FrameIndexTests, NextWrapsAround )
{
    EXPECT_EQ( next_frame_index( 0U, 3U ), 1U );
    EXPECT_EQ( next_frame_index( 2U, 3U ), 0U );
    EXPECT_EQ( next_frame_index( 0U, 1U ), 0U );
}

TEST( FrameIndexTests, PreviousWrapsAround )
{
    EXPECT_EQ( previous_frame_index( 1U, 3U ), 0U );
    EXPECT_EQ( previous_frame_index( 0U, 3U ), 2U );
    EXPECT_EQ( previous_frame_index( 0U, 1U ), 0U );
}

TEST( FrameIndexTests, ZeroFramesDoesNotDivideByZero )
{
    EXPECT_EQ( next_frame_index( 0U, 0U ), 0U );
    EXPECT_EQ( previous_frame_index( 5U, 0U ), 0U );
}

} // namespace
} // namespace ltb::vlk::objs
