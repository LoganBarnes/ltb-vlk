// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/vlk/objs/vulkan_gpu.hpp"

// external
#include <gtest/gtest.h>

namespace ltb::vlk
{

/// \brief Creates a windowless Vulkan instance and device for each test.
///
/// Tests are skipped (not failed) when no Vulkan device is available, so this
/// suite can run anywhere. In CI, Mesa's lavapipe driver provides a software device.
class HeadlessGpuTest : public ::testing::Test
{
protected:
    void SetUp( ) override
    {
        if ( auto const result = gpu_.initialize( { } ); !result )
        {
            GTEST_SKIP( ) << "No usable Vulkan device: " << result.error( ).error_message( );
        }
    }

    objs::VulkanGpu gpu_ = { };
};

} // namespace ltb::vlk
