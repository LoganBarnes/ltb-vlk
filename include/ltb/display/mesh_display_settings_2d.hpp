// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/display/display_modes.hpp"
#include "ltb/geom/mesh.hpp"
#include "ltb/geom/transforms.hpp"

// standard
#include <optional>

namespace ltb::display
{

/// \brief The display options used to display a mesh.
struct MeshDisplaySettings2d
{
    /// \brief true if the mesh is visible.
    bool visible = true;

    /// \brief The geometry style used connect points in the mesh.
    geom::MeshFormat mesh_format = geom::MeshFormat::Triangles;

    /// \brief The color options used to display the mesh.
    ColorMode color_mode = ColorMode::UniformColor;

    /// \brief The color used when the mesh is rendered with ColorMode::UniformColor.
    glm::vec4 uniform_color = glm::vec4( 1.0F );

    /// \brief A collection of transforms to apply to the mesh.
    std::vector< geom::Transform2 > transforms = { };

    /// \brief The number of elements to draw from the mesh. This value
    ///        is clamped to the number of elements in the mesh.
    uint32 draw_count = std::numeric_limits< uint32 >::max( );
};

/// \brief The display options used to display a mesh.
struct PartialMeshDisplaySettings2d
{
    /// \brief true if the mesh is visible.
    std::optional< bool > visible = std::nullopt;

    /// \brief The geometry style used connect points in the mesh.
    std::optional< geom::MeshFormat > mesh_format = std::nullopt;

    /// \brief The color options used to display the mesh.
    std::optional< ColorMode > color_mode = std::nullopt;

    /// \brief The color used when the mesh is rendered with ColorMode::UniformColor.
    std::optional< glm::vec4 > uniform_color = std::nullopt;

    /// \brief The shading mode used to render the mesh.
    std::optional< ShadingMode > shading_mode = std::nullopt;

    /// \brief A collection of transforms to apply to the mesh.
    std::optional< std::vector< geom::Transform2 > > transforms = std::nullopt;

    /// \brief The number of elements to draw from the mesh. This value
    ///        is clamped to the number of elements in the mesh.
    std::optional< uint32 > draw_count = std::nullopt;
};

/// \brief Update the mesh display settings with the provided partial settings.
///        If a field in the partial settings is not set, the corresponding field in the
///        settings will not be updated.
auto update_settings(
    MeshDisplaySettings2d               settings,
    PartialMeshDisplaySettings2d const& partial_settings
) -> MeshDisplaySettings2d;

} // namespace ltb::display
