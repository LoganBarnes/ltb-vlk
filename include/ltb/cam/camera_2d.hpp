// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/cam/camera_render_params.hpp"
#include "ltb/geom/range.hpp"
#include "ltb/utils/types.hpp"

namespace ltb::cam
{

class Camera2d
{
public:
    Camera2d( );

    auto set_center( glm::vec2 center ) -> void;
    auto set_width( float32 width ) -> void;

    auto handle_inputs( ) -> bool;

    auto resize( glm::vec2 const& viewport_size ) -> void;

    [[nodiscard( "Const getter" )]]
    auto render_params( ) const -> CameraRenderParams;

    [[nodiscard( "Const getter" )]]
    auto simple_render_params( ) const -> SimpleCameraRenderParams;

private:
    glm::vec2 world_center_  = glm::vec2( 0.0F );
    float32   world_width_   = 100.0F;
    glm::vec2 viewport_size_ = glm::vec2( 800.0F, 600.0F );

    glm::vec2 center_before_mouse_drag_ = world_center_;

    CameraRenderParams render_params_ = { };

    [[nodiscard( "Const computation" )]]
    auto to_world_pos( glm::vec2 mouse_pos ) const -> glm::vec2;

    auto update_render_params( ) -> void;
};

} // namespace ltb::cam
