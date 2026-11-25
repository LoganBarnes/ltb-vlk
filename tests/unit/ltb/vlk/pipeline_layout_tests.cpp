// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/vlk/pipeline_layout.hpp"

// external
#include <glm/glm.hpp>
#include <gtest/gtest.h>

namespace ltb::vlk
{
namespace
{

struct VertexPushConstants
{
    glm::mat4 transform;
};

struct FragmentPushConstants
{
    glm::vec4 color;
};

TEST( AppendPushConstantRangeTests, FirstRangeStartsAtZero )
{
    auto ranges = std::vector< vk::PushConstantRange >{ };

    append_push_constant_range< VertexPushConstants >( ranges, vk::ShaderStageFlagBits::eVertex );

    ASSERT_EQ( ranges.size( ), 1U );
    EXPECT_EQ( ranges[ 0 ].offset, 0U );
    EXPECT_EQ( ranges[ 0 ].size, sizeof( VertexPushConstants ) );
    EXPECT_EQ( ranges[ 0 ].stageFlags, vk::ShaderStageFlags{ vk::ShaderStageFlagBits::eVertex } );
}

TEST( AppendPushConstantRangeTests, LaterRangesStartAfterThePreviousRange )
{
    auto ranges = std::vector< vk::PushConstantRange >{ };

    append_push_constant_range< VertexPushConstants >( ranges, vk::ShaderStageFlagBits::eVertex );
    append_push_constant_range<
        FragmentPushConstants >( ranges, vk::ShaderStageFlagBits::eFragment );
    append_push_constant_range<
        FragmentPushConstants >( ranges, vk::ShaderStageFlagBits::eFragment );

    ASSERT_EQ( ranges.size( ), 3U );
    EXPECT_EQ( ranges[ 1 ].offset, sizeof( VertexPushConstants ) );
    EXPECT_EQ(
        ranges[ 2 ].offset,
        sizeof( VertexPushConstants ) + sizeof( FragmentPushConstants )
    );
}

} // namespace
} // namespace ltb::vlk
