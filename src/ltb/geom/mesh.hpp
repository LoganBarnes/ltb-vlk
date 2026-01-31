
// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/geom/fwd.hpp"

// external
#include <glm/glm.hpp>

// standard
#include <vector>

namespace ltb::geom
{

enum class MeshFormat
{
    Points,
    Lines,
    LineStrip,
    Triangles,
    TriangleStrip,
};

template < glm::length_t N, typename T >
struct Mesh
{
    static_assert( std::is_floating_point_v< T >, "Mesh only supports floating point types." );

    using PositionType = glm::vec< N, T >;
    using NormalType   = glm::vec< N, T >;
    using UvType       = glm::vec< N - 1, T >;
    using ColorType    = glm::vec3;
    using IndexType    = uint32;

    MeshFormat format = MeshFormat::Triangles;

    std::vector< PositionType > positions     = { };
    std::vector< NormalType >   normals       = { };
    std::vector< UvType >       uvs           = { };
    std::vector< ColorType >    vertex_colors = { };

    std::vector< IndexType > indices = { };
};

} // namespace ltb::geom
