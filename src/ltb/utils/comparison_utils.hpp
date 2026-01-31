// ///////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022 Logan Barnes - All Rights Reserved
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// standard
#include <type_traits>

namespace ltb::utils
{

template < typename Float >
    requires std::is_floating_point_v< Float >
auto exactly_equal( Float const a, Float const b ) -> bool
{
    return a == b;
}

} // namespace ltb::utils
