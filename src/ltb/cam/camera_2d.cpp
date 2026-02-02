// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/cam/camera_2d.hpp"

// project
#include "ltb/gui/imgui.hpp"
#include "ltb/utils/comparison_utils.hpp"

// standard
#include <algorithm>

namespace ltb::cam
{
namespace
{

[[nodiscard( "Const computation" )]]
auto compute_half_view_size( glm::vec2 const& viewport_size, float32 const width ) -> glm::vec2
{
    auto const aspect_ratio     = viewport_size.x / viewport_size.y;
    auto const camera_view_size = glm::vec2{ width, width / aspect_ratio };
    return camera_view_size * 0.5F;
}

} // namespace

Camera2d::Camera2d( )
{
    update_render_params( );
}

auto Camera2d::set_center( glm::vec2 const center ) -> void
{
    world_center_ = center;
    update_render_params( );
}

auto Camera2d::set_width( float32 const width ) -> void
{
    world_width_ = width;
    update_render_params( );
}

auto Camera2d::handle_inputs( ) -> bool
{
    auto camera_changed = false;

    auto const& io = ImGui::GetIO( );

    if ( !utils::exactly_equal( io.MouseWheel, 0.0F ) )
    {
        auto const framebuffer_scale = glm::vec2{ io.DisplayFramebufferScale };
        auto const mouse_pos         = glm::vec2{ io.MousePos } * framebuffer_scale;
        auto const world_pos_before  = to_world_pos( mouse_pos );

        world_width_ *= std::pow( 0.9F, io.MouseWheel );

        auto const world_pos_after = to_world_pos( mouse_pos );
        auto const world_delta     = world_pos_before - world_pos_after;

        world_center_ += world_delta;

        camera_changed = true;
    }

    if ( ImGui::IsMouseDragging( ImGuiMouseButton_Right ) )
    {
        auto const mouse_delta = glm::vec2{ ImGui::GetMouseDragDelta( ImGuiMouseButton_Right ) };
        auto const framebuffer_scale  = glm::vec2{ io.DisplayFramebufferScale };
        auto const scaled_mouse_delta = mouse_delta * framebuffer_scale;

        auto const clip_delta     = ( scaled_mouse_delta / viewport_size_ ) * 2.0F;
        auto const half_view_size = compute_half_view_size( viewport_size_, world_width_ );
        auto const world_delta    = clip_delta * half_view_size;

        world_center_  = center_before_mouse_drag_ - world_delta;
        camera_changed = true;
    }
    else
    {
        center_before_mouse_drag_ = world_center_;
    }

    if ( camera_changed )
    {
        update_render_params( );
    }

    return camera_changed;
}

auto Camera2d::resize( glm::vec2 const& viewport_size ) -> void
{
    viewport_size_ = viewport_size;
    update_render_params( );
}

auto Camera2d::render_params( ) const -> CameraRenderParams
{
    return render_params_;
}

auto Camera2d::simple_render_params( ) const -> SimpleCameraRenderParams
{
    return SimpleCameraRenderParams{
        .clip_from_world = render_params_.clip_from_world,
        .world_from_clip = render_params_.world_from_clip,
    };
}

auto Camera2d::to_world_pos( glm::vec2 const mouse_pos ) const -> glm::vec2
{
    auto const clip_pos       = ( ( mouse_pos / viewport_size_ ) * 2.0F ) - 1.0F;
    auto const half_view_size = compute_half_view_size( viewport_size_, world_width_ );
    return world_center_ + ( clip_pos * half_view_size );
}

auto Camera2d::update_render_params( ) -> void
{
    auto const half_view_size = compute_half_view_size( viewport_size_, world_width_ );

    constexpr auto cam_distance = 2.0F;

    render_params_.view_from_world = glm::lookAt(
        glm::vec3( world_center_, -cam_distance ),
        glm::vec3( world_center_, 0.0F ),
        glm::vec3( 0.0F, 1.0F, 0.0F )
    );
    render_params_.clip_from_view = glm::ortho(
        +half_view_size.x,
        -half_view_size.x,
        +half_view_size.y,
        -half_view_size.y,
        cam_distance - 1.0F,
        cam_distance + 1.0F
    );

    render_params_.clip_from_world = render_params_.clip_from_view * render_params_.view_from_world;
    render_params_.world_from_clip = glm::inverse( render_params_.clip_from_world );
}

} // namespace ltb::cam
