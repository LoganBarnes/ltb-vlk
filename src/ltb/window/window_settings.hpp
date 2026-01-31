// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// external
#include <glm/glm.hpp>

// standard
#include <optional>
#include <string>

namespace ltb::window
{

constexpr auto default_width  = 1280U;
constexpr auto default_height = 720U;

/// \brief Settings used to initialize an operating system window.
struct WindowSettings
{
    /// \brief The title displayed in the window's title bar.
    std::string title = "Window";

    /// \brief If true, the alpha channel of the window will be transparent.
    bool transparent_background = false;

    /// \brief If true, the window will be resizable.
    bool resizable = true;

    /// \brief If true, the window will contain a title_bar.
    bool title_bar = true;

    /// \brief The initial size of the window. Nullopt implies fullscreen.
    std::optional< glm::ivec2 > initial_size = glm::ivec2{ default_width, default_height };
};

} // namespace ltb::window
