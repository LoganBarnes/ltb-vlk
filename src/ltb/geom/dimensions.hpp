// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// external
#include <glm/glm.hpp>

namespace ltb::geom
{

constexpr auto one_dimension    = glm::length_t{ 1 };
constexpr auto two_dimensions   = glm::length_t{ 2 };
constexpr auto three_dimensions = glm::length_t{ 3 };
constexpr auto four_dimensions  = glm::length_t{ 4 };

template < glm::length_t Dimensions >
concept TwoOrThreeD = ( two_dimensions == Dimensions ) || ( three_dimensions == Dimensions );

template < glm::length_t Dimensions >
concept ThreeOrFourD = ( three_dimensions == Dimensions ) || ( four_dimensions == Dimensions );

} // namespace ltb::geom
