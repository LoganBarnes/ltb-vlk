// ///////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022 Logan Barnes - All Rights Reserved
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/utils/string.hpp"

// standard
#include <algorithm>

namespace ltb::utils
{

auto to_lower_ascii( std::string const& str ) -> std::string
{
    // Only 'A'-'Z' change. std::tolower depends on the C locale, so it isn't used here.
    auto result = str;
    std::ranges::transform( result, result.begin( ), []( char const c ) -> char {
        return ( ( c >= 'A' ) && ( c <= 'Z' ) ) ? static_cast< char >( c - 'A' + 'a' ) : c;
    } );
    return result;
}

} // namespace ltb::utils
