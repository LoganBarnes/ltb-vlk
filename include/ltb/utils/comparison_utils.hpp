// ///////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022 Logan Barnes - All Rights Reserved
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// external
#include <google/AlmostEquals.h>

// standard
#include <type_traits>

namespace ltb::utils
{

template < typename Float >
    requires std::is_floating_point_v< Float >
auto constexpr almost_equal( Float const a, Float const b ) -> bool
{
    return AlmostEquals( a, b );
}

template < typename Float >
    requires std::is_floating_point_v< Float >
auto exactly_equal( Float const a, Float const b ) -> bool
{
    return a == b;
}

} // namespace ltb::utils
