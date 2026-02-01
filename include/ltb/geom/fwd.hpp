// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/utils/types.hpp"

// external
#include <glm/glm.hpp>

namespace ltb::geom
{

template < glm::length_t N, typename T >
struct Mesh;

using Mesh3 = Mesh< 3, float32 >;
using Mesh2 = Mesh< 2, float32 >;

template < typename T >
struct Range;

using Range2i = Range< glm::ivec2 >;
using Range2  = Range< glm::vec2 >;

} // namespace ltb::geom
