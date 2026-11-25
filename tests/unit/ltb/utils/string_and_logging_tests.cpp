// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/utils/logging.hpp"
#include "ltb/utils/string.hpp"

// external
#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

namespace ltb::utils
{
namespace
{

TEST( StringTests, ToLowerAsciiOnlyChangesAsciiLetters )
{
    EXPECT_EQ( to_lower_ascii( "Hello, WORLD 123!" ), "hello, world 123!" );
    EXPECT_EQ( to_lower_ascii( "" ), "" );
    EXPECT_EQ( to_lower_ascii( "\xC3\x89" ), "\xC3\x89" ); // UTF-8 'É' is left alone
}

class LogLevelTest : public ::testing::Test
{
protected:
    void TearDown( ) override
    {
        spdlog::set_level( original_level_ );
    }

    spdlog::level::level_enum original_level_ = spdlog::get_level( );
};

TEST_F( LogLevelTest, AcceptsFullAndShortNamesInAnyCase )
{
    EXPECT_TRUE( try_setting_log_level( "warning" ).has_value( ) );
    EXPECT_EQ( spdlog::get_level( ), spdlog::level::warn );

    EXPECT_TRUE( try_setting_log_level( "ERR" ).has_value( ) );
    EXPECT_EQ( spdlog::get_level( ), spdlog::level::err );

    EXPECT_TRUE( try_setting_log_level( "Debug" ).has_value( ) );
    EXPECT_EQ( spdlog::get_level( ), spdlog::level::debug );

    EXPECT_TRUE( try_setting_log_level( "off" ).has_value( ) );
    EXPECT_EQ( spdlog::get_level( ), spdlog::level::off );
}

TEST_F( LogLevelTest, RejectsUnknownNamesWithoutChangingTheLevel )
{
    spdlog::set_level( spdlog::level::info );

    EXPECT_FALSE( try_setting_log_level( "verbose" ).has_value( ) );
    EXPECT_EQ( spdlog::get_level( ), spdlog::level::info );
}

} // namespace
} // namespace ltb::utils
