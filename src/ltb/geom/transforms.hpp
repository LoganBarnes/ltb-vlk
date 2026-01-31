// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/geom/dimensions.hpp"
#include "ltb/utils/types.hpp"

// external
#include <glm/detail/type_quat.hpp>
#include <glm/gtx/matrix_transform_2d.hpp>

// standard
#include <variant>
#include <vector>

namespace ltb::geom
{

template < glm::length_t Dimensions >
    requires TwoOrThreeD< Dimensions >
struct Translation;

template < glm::length_t Dimensions >
    requires TwoOrThreeD< Dimensions >
struct Scale;

template < glm::length_t Dimensions >
    requires TwoOrThreeD< Dimensions >
struct RotationVec;

struct RotationQuat3;

// 2D
using Translation2 = Translation< two_dimensions >;
using Scale2       = Scale< two_dimensions >;
using RotationVec2 = RotationVec< two_dimensions >;
using Transform2   = std::variant< Translation2, Scale2, RotationVec2 >;

// 3D
using Translation3 = Translation< three_dimensions >;
using Scale3       = Scale< three_dimensions >;
using RotationVec3 = RotationVec< three_dimensions >;
using Transform3   = std::variant< Translation3, Scale3, RotationVec3, RotationQuat3 >;

template < glm::length_t Dimensions >
using Mat = glm::mat< Dimensions, Dimensions, float32 >;

/// \brief Concept to ensure that the provided Dimensions match the Transform type.
template < glm::length_t Dimensions, typename Transform >
concept DimensionsMatchTransform
    = ( ( Dimensions == two_dimensions ) && std::is_same_v< Transform, Transform2 > )
   || ( ( Dimensions == three_dimensions ) && std::is_same_v< Transform, Transform3 > );

/// \brief Consolidates a collection of 2D transforms into a single 3x3 transformation matrix.
///        Transforms will be applied in the order they are provided:
///        T_final = T_n * T_n-1 * ... * T_1 * T_0
auto consolidate_transforms( std::vector< Transform2 > const& transforms ) -> glm::mat3;

/// \brief Consolidates a collection of 3D transforms into a single 4x4 transformation matrix.
///        Transforms will be applied in the order they are provided:
///        T_final = T_n * T_n-1 * ... * T_1 * T_0
auto consolidate_transforms( std::vector< Transform3 > const& transforms ) -> glm::mat4;

/// \brief Converts a 3D world transformation matrix to a 3x3 matrix for transforming normals.
auto transform_for_normals( glm::mat4 const& world_from_local ) -> glm::mat3;

// Convert transforms to homogeneous transformation matrices:

template < glm::length_t Dimensions >
    requires TwoOrThreeD< Dimensions >
auto to_mat( Translation< Dimensions > const& translation ) -> Mat< Dimensions + 1 >;

template < glm::length_t Dimensions >
    requires TwoOrThreeD< Dimensions >
auto to_mat( Scale< Dimensions > const& scale ) -> Mat< Dimensions + 1 >;

template < glm::length_t Dimensions >
    requires TwoOrThreeD< Dimensions >
auto to_mat( RotationVec< Dimensions > const& rotation ) -> Mat< Dimensions + 1 >;

auto to_mat( RotationQuat3 const& rotation ) -> glm::mat4;

// ////////////////// Transform Implementations Below ////////////////// //

template < glm::length_t Dimensions >
    requires TwoOrThreeD< Dimensions >
struct Translation
{
    glm::vec< Dimensions, float32 > offset = glm::vec< Dimensions, float32 >( 0.0F );
};

template <>
struct RotationVec< two_dimensions >
{
    float32 angle_radians = 0.0F;
};

template <>
struct RotationVec< three_dimensions >
{
    float32   angle_radians   = 0.0F;
    glm::vec3 normalized_axis = glm::vec3( 0.0F, 0.0F, 1.0F );
};

template < glm::length_t Dimensions >
    requires TwoOrThreeD< Dimensions >
struct Scale
{
    glm::vec< Dimensions, float32 > scale = glm::vec< Dimensions, float32 >( 1.0F );
};

struct RotationQuat3
{
    glm::vec4 quat_coeffs = glm::vec4{ 1.0F, 0.0F, 0.0F, 0.0F };
};

} // namespace ltb::geom
