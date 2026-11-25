// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/utils/initializable.hpp"

// external
#include <gtest/gtest.h>

namespace ltb::utils
{
namespace
{

struct Counter
{
    int  initialize_calls = 0;
    bool initialized      = false;

    auto initialize( ) -> Result< void >
    {
        ++initialize_calls;
        initialized = true;
        return success( );
    }

    [[nodiscard]] auto is_initialized( ) const -> bool
    {
        return initialized;
    }
};

static_assert( Initializable< Counter > );

TEST( InitializableTests, InitializeOnlyRunsOnce )
{
    auto counter = Counter{ };

    EXPECT_FALSE( IsInitialized{ }( counter ) );
    EXPECT_TRUE( Initialize{ }( counter ).has_value( ) );
    EXPECT_TRUE( Initialize{ }( counter ).has_value( ) );

    EXPECT_TRUE( IsInitialized{ }( counter ) );
    EXPECT_EQ( counter.initialize_calls, 1 );
}

} // namespace
} // namespace ltb::utils
