// ///////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022 Logan Barnes - All Rights Reserved
// ///////////////////////////////////////////////////////////////////////////////////////
#include "string.hpp"

// standard
#include <algorithm>

namespace ltb::utils
{

auto to_lower_ascii( std::string const& str ) -> std::string
{
    auto result = str;
    std::transform( str.begin( ), str.end( ), result.begin( ), []( unsigned char c ) {
        return std::tolower( c );
    } );
    return result;
}

} // namespace ltb::utils
