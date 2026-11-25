// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/utils/units.hpp"

// external
#include <gtest/gtest.h>

namespace ltb::utils
{
namespace
{

TEST( UnitsTests, TemperatureConversionsWithFloatingPoint )
{
    EXPECT_DOUBLE_EQ( c_from_f( 212.0 ), 100.0 );
    EXPECT_DOUBLE_EQ( f_from_c( 100.0 ), 212.0 );
    EXPECT_DOUBLE_EQ( k_from_c( 0.0 ), 273.15 );
    EXPECT_DOUBLE_EQ( c_from_k( 273.15 ), 0.0 );
    EXPECT_FLOAT_EQ( c_from_f( -40.0F ), -40.0F );
}

TEST( UnitsTests, TemperatureConversionsWithIntegers )
{
    EXPECT_EQ( c_from_f( 212 ), 100 );
    EXPECT_EQ( f_from_c( 100 ), 212 );
    EXPECT_EQ( c_from_milli_c( 25'000 ), 25 );

    // Integer inputs can produce floating point outputs.
    EXPECT_DOUBLE_EQ( ( k_from_c< int, double >( 0 ) ), 273.15 );
}

TEST( UnitsTests, PercentagesWithIntegers )
{
    EXPECT_EQ( decimal_from_percentage( 300 ), 3 );
    EXPECT_EQ( percentage_from_decimal( 3 ), 300 );
    EXPECT_DOUBLE_EQ( decimal_from_percentage( 50.0 ), 0.5 );
}

} // namespace
} // namespace ltb::utils
