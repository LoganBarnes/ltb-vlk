// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// external
#include <glm/glm.hpp>

namespace ltb::geom
{

constexpr auto zero_vec2 = glm::vec2( 0.0F );
constexpr auto x_axis2   = glm::vec2( 1.0F, 0.0F );
constexpr auto y_axis2   = glm::vec2( 0.0F, 1.0F );

constexpr auto zero_vec3 = glm::vec3( 0.0F );
constexpr auto x_axis3   = glm::vec3( x_axis2, 0.0F );
constexpr auto y_axis3   = glm::vec3( y_axis2, 0.0F );
constexpr auto z_axis3   = glm::vec3( 0.0F, 0.0F, 1.0F );

constexpr auto zero_vec4 = glm::vec4( 0.0F );
constexpr auto x_axis4   = glm::vec4( x_axis3, 0.0F );
constexpr auto y_axis4   = glm::vec4( y_axis3, 0.0F );
constexpr auto z_axis4   = glm::vec4( z_axis3, 0.0F );
constexpr auto w_axis4   = glm::vec4( 0.0F, 0.0F, 0.0F, 1.0F );

} // namespace ltb::geom
