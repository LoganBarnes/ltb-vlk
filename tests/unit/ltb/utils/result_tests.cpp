// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/utils/result.hpp"

// external
#include <gtest/gtest.h>

// standard
#include <string>

namespace ltb::utils
{
namespace
{

auto make_int( bool const succeed ) -> Result< int >
{
    if ( succeed )
    {
        return 42;
    }
    return LTB_MAKE_UNEXPECTED_ERROR( "no int" );
}

// Returns the error message, or "ok" when the checked value is valid.
auto message_or_ok( Error const& error ) -> std::string
{
    return error.error_message( );
}

auto check_valid_or( bool const valid ) -> std::string
{
    LTB_CHECK_VALID_OR( valid, message_or_ok );
    return "ok";
}

auto check_valid_or_with_message( bool const valid ) -> std::string
{
    LTB_CHECK_VALID_OR( valid, "extra", message_or_ok );
    return "ok";
}

auto check_or( bool const succeed ) -> std::string
{
    LTB_CHECK_OR( make_int( succeed ), message_or_ok );
    return "ok";
}

auto check_or_assign( bool const succeed ) -> std::string
{
    LTB_CHECK_OR( auto const value, make_int( succeed ), message_or_ok );
    return std::to_string( value );
}

auto check_assign( bool const succeed ) -> Result< int >
{
    LTB_CHECK( auto const value, make_int( succeed ) );
    return value + 1;
}

TEST( ResultMacroTests, CheckPropagatesErrorsAndAssignsValues )
{
    EXPECT_EQ( check_assign( true ).value( ), 43 );

    auto const failed = check_assign( false );
    ASSERT_FALSE( failed.has_value( ) );
    EXPECT_EQ( failed.error( ).error_message( ), "no int" );
}

TEST( ResultMacroTests, CheckValidOrReturnsCallbackResult )
{
    EXPECT_EQ( check_valid_or( true ), "ok" );
    EXPECT_EQ( check_valid_or( false ), "valid invalid" );
    EXPECT_EQ( check_valid_or_with_message( false ), "valid invalid: extra" );
}

TEST( ResultMacroTests, CheckOrReturnsCallbackResult )
{
    EXPECT_EQ( check_or( true ), "ok" );
    EXPECT_EQ( check_or( false ), "no int" );
}

TEST( ResultMacroTests, CheckOrAssignsOnSuccessAndReturnsCallbackResultOnError )
{
    EXPECT_EQ( check_or_assign( true ), "42" );
    EXPECT_EQ( check_or_assign( false ), "no int" );
}

} // namespace
} // namespace ltb::utils
