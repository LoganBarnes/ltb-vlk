// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/window/glfw_utils.hpp"

namespace ltb::window
{

/// \brief A context for the GLFW library.
class GlfwContext
{
public:
    /// \brief Creates an uninitialized instance.
    GlfwContext( ) = default;

    /// \brief Initializes the GLFW library.
    auto initialize( ) -> utils::Result< void >;

    /// \brief Returns true if the GLFW library was initialized successfully.
    [[nodiscard( "Const getter" )]]
    auto is_initialized( ) const -> bool;

    /// \brief Polls for input events and returns immediately.
    auto poll_events( ) const -> void;

    /// \brief Blocks until input events are received.
    auto wait_for_events( ) const -> void;

    /// \brief Returns a list of required vk::Instance extensions.
    [[nodiscard( "Const getter" )]]
    auto get_vulkan_instance_extensions( ) const -> std::vector< char const* >;

private:
    /// \brief RAII object to handle a GLFW context.
    GlfwHandle glfw_ = nullptr;
};

} // namespace ltb::window
