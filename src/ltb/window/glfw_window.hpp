// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/window/fwd.hpp"
#include "ltb/window/glfw_utils.hpp"

namespace ltb::window
{

/// \brief An operating system window created with the GLFW framework.
class GlfwWindow
{
public:
    /// \brief Creates a window with the specified settings.
    explicit GlfwWindow( GlfwContext& context, WindowSettings settings );

    /// \brief Initializes the window.
    auto initialize( ) -> utils::Result< void >;

    /// \brief Returns true if the window was initialized successfully.
    [[nodiscard( "Const getter" )]]
    auto is_initialized( ) const -> bool;

    /// \brief Returns true if the window requested to close.
    [[nodiscard( "Const getter" )]]
    auto should_close( ) const -> bool;

    /// \brief Returns the new size of the framebuffer if the window was resized.
    [[nodiscard( "Const getter" )]]
    auto resized( ) const -> std::optional< glm::ivec2 >;

    /// \brief Resets the callback data, clearing any stored events (such as resize values).
    auto reset_callback_data( ) const -> void;

    /// \brief Returns the current size of the framebuffer.
    [[nodiscard( "Const getter" )]]
    auto framebuffer_size( ) const -> glm::ivec2;

    /// \brief The raw GLFW window handle. This will be null if
    ///        the window was not initialized successfully.
    [[nodiscard( "Const getter" )]]
    auto get( ) const -> GLFWwindow*;

private:
    GlfwContext&         context_;
    WindowSettings const settings_;

    /// \brief RAII object to handle a GLFW window.
    GlfwWindowHandle window_ = nullptr;

    /// \brief The data passed to GLFW callbacks.
    /// \details CallbackData is its own struct and stored as a smart pointer because GLFW
    ///          requires a raw pointer to the data. If callback_data was stored as a local
    ///          member variable and this class was moved, the pointer would become invalid.
    std::unique_ptr< CallbackData > callback_data_ = nullptr;
};

} // namespace ltb::window
