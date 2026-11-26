// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/vlk/version.hpp"

// external
#include <gtest/gtest.h>

// The Vulkan SDK's version macros use C-style casts, which Apple Clang reports even though
// they come from a system header.
#if defined( __clang__ ) && defined( __apple_build_version__ )
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif

namespace
{

// These are compile-time checks; the test exists so the suite reports them.
static_assert( VLK_MAKE_API_VERSION( 0, 1, 2, 3 ) == VK_MAKE_API_VERSION( 0, 1, 2, 3 ) );
static_assert( VLK_MAKE_VERSION( 1, 2, 3 ) == VK_MAKE_API_VERSION( 0, 1, 2, 3 ) );

#if defined( VK_API_VERSION_1_4 )
static_assert( VLK_API_VERSION_1_4 == VK_API_VERSION_1_4 );
#endif

TEST( VersionTests, ApiVersion14IsVersion14 )
{
    EXPECT_EQ( VK_API_VERSION_MAJOR( VLK_API_VERSION_1_4 ), 1U );
    EXPECT_EQ( VK_API_VERSION_MINOR( VLK_API_VERSION_1_4 ), 4U );
    EXPECT_EQ( VK_API_VERSION_PATCH( VLK_API_VERSION_1_4 ), 0U );
}

} // namespace
