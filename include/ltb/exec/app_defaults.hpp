// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/utils/types.hpp"

// standard
#include <limits>

namespace ltb::exec
{

constexpr auto max_frames_in_flight = 3U;
constexpr auto max_possible_timeout = std::numeric_limits< uint64 >::max( );

} // namespace ltb::exec
