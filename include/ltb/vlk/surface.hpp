// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/utils/result.hpp"
#include "ltb/vlk/fwd.hpp"
#include "ltb/vlk/vulkan.hpp"
#include "ltb/window/glfw_window.hpp"

namespace ltb::vlk
{

class Surface
{
public:
    Surface( window::GlfwWindow& glfw_window, Instance& instance );

    auto initialize( ) -> utils::Result< void >;

    [[nodiscard( "Const getter" )]]
    auto is_initialized( ) const -> bool;

    [[nodiscard( "Const getter" )]]
    auto get( ) const -> vk::SurfaceKHR const&;
    auto get( ) -> vk::SurfaceKHR&;

    [[nodiscard( "Const getter" )]]
    auto framebuffer_size( ) const -> vk::Extent2D;

private:
    window::GlfwWindow& glfw_window_;
    Instance&           instance_;

    vk::UniqueSurfaceKHR surface_ = { };
};

} // namespace ltb::vlk
