// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/utils/result.hpp"
#include "ltb/utils/types.hpp"
#include "ltb/vlk/vulkan.hpp"
#include "ltb/window/window_settings.hpp"

// external
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

// standard
#include <memory>

namespace ltb::window
{

/// \brief A RAII wrapper for GLFW library initialization.
class ScopedGlfw
{
public:
    explicit ScopedGlfw( );
    ~ScopedGlfw( );

    /// \brief The return value of glfwInit(), GLFW_FALSE if initialization failed.
    int32 init_value = GLFW_FALSE;
};

/// \brief A custom deleter for GLFW windows that ensures the window is destroyed properly.
struct GlfwWindowDeleter
{
    auto operator( )( GLFWwindow* window ) const -> void;
};

/// \brief A unique pointer type for ScopedGlfw that ensures GLFW is initialized and cleaned up.
using GlfwHandle = std::unique_ptr< ScopedGlfw >;

/// \brief A unique pointer type for GLFW windows that ensures they are destroyed properly.
using GlfwWindowHandle = std::unique_ptr< GLFWwindow, GlfwWindowDeleter >;

/// \brief Applies the specified window settings as GLFW window hints.
auto apply_window_hints( WindowSettings const& settings ) -> utils::Result< void >;

/// \brief Determines the initial size of the window based on the settings and monitor.
auto get_initial_window_size( WindowSettings const& settings ) -> utils::Result< glm::ivec2 >;

struct CallbackData
{
    /// \brief This gets set by the window resize callback
    ///        when the window is resized. It should be
    ///        cleared before the window events are polled.
    std::optional< glm::ivec2 > resized_framebuffer = std::nullopt;
};

/// \brief Registers the GLFW callbacks so they set the relevant CallbackData fields.
auto set_callbacks( GLFWwindow* window ) -> utils::Result< std::unique_ptr< CallbackData > >;

} // namespace ltb::window
