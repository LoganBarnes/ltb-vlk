// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/geom/transforms.hpp"

// external
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_transform_2d.hpp>
#include <glm/gtx/quaternion.hpp>
#include <spdlog/spdlog.h>

// standard
#include <numeric>

namespace ltb::geom
{
namespace
{

struct TransformToMatVisitor
{
    auto operator( )( auto const& transform ) const
    {
        return to_mat( transform );
    }
};

struct TransformToMat
{
    template < typename Transform >
    auto operator( )( Transform const& transform ) const
    {
        return std::visit( TransformToMatVisitor{ }, transform );
    }
};

template < glm::length_t Dimensions, typename Transform >
    requires DimensionsMatchTransform< Dimensions, Transform >
auto consolidate_transforms_impl( std::vector< Transform > const& transforms )
{
    using Matrix = Mat< Dimensions + 1 >;

    constexpr auto initial_value       = glm::identity< Matrix >( );
    constexpr auto transform_operation = TransformToMat{ };
    constexpr auto reduce_operation    = std::multiplies< Matrix >{ };

    return std::transform_reduce(
        transforms.rbegin( ),
        transforms.rend( ),
        initial_value,
        reduce_operation,
        transform_operation
    );
}

} // namespace

auto consolidate_transforms( std::vector< Transform2 > const& transforms ) -> glm::mat3
{
    return consolidate_transforms_impl< two_dimensions >( transforms );
}

auto consolidate_transforms( std::vector< Transform3 > const& transforms ) -> glm::mat4
{
    return consolidate_transforms_impl< three_dimensions >( transforms );
}

template < glm::length_t Dimensions >
    requires TwoOrThreeD< Dimensions >
auto to_mat( Translation< Dimensions > const& translation ) -> Mat< Dimensions + 1 >
{
    using Matrix = Mat< Dimensions + 1 >;
    return glm::translate( glm::identity< Matrix >( ), translation.offset );
}

template < glm::length_t Dimensions >
    requires TwoOrThreeD< Dimensions >
auto to_mat( RotationVec< Dimensions > const& rotation ) -> Mat< Dimensions + 1 >
{
    using Matrix            = Mat< Dimensions + 1 >;
    constexpr auto identity = glm::identity< Matrix >( );

    if constexpr ( Dimensions == two_dimensions )
    {
        return glm::rotate( identity, rotation.angle_radians );
    }
    else if constexpr ( Dimensions == three_dimensions )
    {
        return glm::rotate( identity, rotation.angle_radians, rotation.normalized_axis );
    }
    else
    {
        static_assert( TwoOrThreeD< Dimensions >, "Dimensions must be 2 or 3" );
    }
}

template < glm::length_t Dimensions >
    requires TwoOrThreeD< Dimensions >
auto to_mat( Scale< Dimensions > const& scale ) -> Mat< Dimensions + 1 >
{
    using Matrix = Mat< Dimensions + 1 >;
    return glm::scale( glm::identity< Matrix >( ), scale.scale );
}

auto to_mat( RotationQuat3 const& rotation ) -> glm::mat4
{
    auto const quat = glm::quat(
        rotation.quat_coeffs[ 0 ],
        rotation.quat_coeffs[ 1 ],
        rotation.quat_coeffs[ 2 ],
        rotation.quat_coeffs[ 3 ]
    );
    return glm::toMat4( quat );
}

// 2D
template auto to_mat( Translation2 const& ) -> glm::mat3;
template auto to_mat( RotationVec2 const& ) -> glm::mat3;
template auto to_mat( Scale2 const& ) -> glm::mat3;

// 3D
template auto to_mat( Translation3 const& ) -> glm::mat4;
template auto to_mat( RotationVec3 const& ) -> glm::mat4;
template auto to_mat( Scale3 const& ) -> glm::mat4;

auto transform_for_normals( glm::mat4 const& world_from_local ) -> glm::mat3
{
    return glm::transpose( glm::inverse( glm::mat3( world_from_local ) ) );
}

} // namespace ltb::geom
