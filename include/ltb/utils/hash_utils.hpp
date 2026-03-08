// ///////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022 Logan Barnes - All Rights Reserved
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/utils/types.hpp"

// standard
#include <functional>
#include <string>

namespace ltb::utils
{

template < class T >
auto hash_combine( std::size_t seed, T const& v ) -> std::size_t
{
    return seed ^ ( std::hash< T >{ }( v ) + 0x9e3779b9 + ( seed << 6 ) + ( seed >> 2 ) );
}

/// \brief Convert a string to a uint32 using `std::seed_seq`.
auto string_seed_to_uint( std::string const& str ) -> uint32;

} // namespace ltb::utils
