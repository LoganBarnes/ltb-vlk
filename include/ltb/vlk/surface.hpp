// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/utils/result.hpp"
#include "ltb/vlk/fwd.hpp"
#include "ltb/vlk/vulkan.hpp"
#include "ltb/window/fwd.hpp"

struct GLFWwindow;

namespace ltb::vlk
{

class Surface
{
public:
    Surface( window::GlfwWindow& glfw_window, Instance& instance );

    /// \brief A surface for a window this library doesn't own. The window must have been
    ///        created with GLFW_CLIENT_API set to GLFW_NO_API and has to outlive the surface.
    Surface( GLFWwindow* window, Instance& instance );

    auto initialize( ) -> utils::Result< void >;

    [[nodiscard( "Const getter" )]]
    auto is_initialized( ) const -> bool;

    [[nodiscard( "Const getter" )]]
    auto get( ) const -> vk::SurfaceKHR const&;
    auto get( ) -> vk::SurfaceKHR&;

    [[nodiscard( "Const getter" )]]
    auto framebuffer_size( ) const -> vk::Extent2D;

private:
    /// \brief Null if the surface was created from an external window.
    window::GlfwWindow* glfw_window_     = nullptr;
    GLFWwindow*         external_window_ = nullptr;
    Instance&           instance_;

    [[nodiscard( "Const getter" )]]
    auto window( ) const -> GLFWwindow*;

    vk::UniqueSurfaceKHR surface_ = { };
};

} // namespace ltb::vlk
