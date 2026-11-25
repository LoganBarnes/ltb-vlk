// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/vlk/objs/vulkan_gpu.hpp"

// external
#include <gtest/gtest.h>

// standard
#include <cstdlib>
#include <string_view>

namespace ltb::vlk
{

/// \brief Creates a windowless Vulkan instance and device for each test.
///
/// Tests are skipped (not failed) when no Vulkan device is available, so this
/// suite can run anywhere. Set `LTB_VLK_REQUIRE_GPU=1` to fail instead (CI does,
/// using Mesa's lavapipe software driver, so the GPU tests can't silently stop running).
class HeadlessGpuTest : public ::testing::Test
{
protected:
    void SetUp( ) override
    {
        if ( auto const result = gpu_.initialize( { } ); !result )
        {
            auto const* const require_gpu = std::getenv( "LTB_VLK_REQUIRE_GPU" );
            if ( ( nullptr != require_gpu ) && ( std::string_view{ require_gpu } == "1" ) )
            {
                FAIL( ) << "No usable Vulkan device (LTB_VLK_REQUIRE_GPU=1): "
                        << result.error( ).error_message( );
            }
            GTEST_SKIP( ) << "No usable Vulkan device: " << result.error( ).error_message( );
        }
    }

    objs::VulkanGpu gpu_ = { };
};

} // namespace ltb::vlk
