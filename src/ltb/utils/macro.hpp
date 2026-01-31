// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// One level of macro indirection is required in order to resolve
// __COUNTER__, and get varname1 instead of varname__COUNTER__.
#define LTB_CONCAT_INNER( a, b ) a##b
#define LTB_CONCAT( a, b ) LTB_CONCAT_INNER( a, b )
#define LTB_UNIQUE_NAME( base ) LTB_CONCAT( base, __COUNTER__ )

#define LTB_EXPAND( x ) x

#define LTB_GET2( _1, _2, NAME, ... ) NAME
#define LTB_GET3( _1, _2, _3, NAME, ... ) NAME

#include <string_view>

#define LTB_STRINGIFY( item, ... ) "" #item
#define LTB_IS_DEFINED( macro )                                                                    \
    std::string_view( "" #macro ) != std::string_view( LTB_STRINGIFY( macro, ) )
