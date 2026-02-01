// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/gui/imgui_context.hpp"
#include "ltb/gui/imgui_glfw.hpp"
#include "ltb/gui/imgui_setup.hpp"
#include "ltb/gui/imgui_vulkan.hpp"
#include "ltb/vlk/objs/fwd.hpp"
#include "ltb/window/fwd.hpp"

namespace ltb::gui
{

enum class DpiFontHandling
{
    /// \brief Ignore the DPI scaling and use the default font size.
    /// \note  Fonts may appear too small on high-DPI screens.
    Ignore,
    /// \brief Scale the fonts based on the primary monitor's DPI.
    Scale,
};

class ImguiGlfwVulkanSetup
{
public:
    ImguiGlfwVulkanSetup(
        window::GlfwWindow&            glfw_window,
        vlk::objs::VulkanGpu&          gpu,
        vlk::objs::VulkanPresentation& presentation
    );
    ImguiGlfwVulkanSetup(
        window::GlfwWindow&            glfw_window,
        vlk::objs::VulkanGpu&          gpu,
        vlk::objs::VulkanPresentation& presentation,
        DpiFontHandling                dpi_handling
    );

    auto initialize( ) -> utils::Result< void >;

    [[nodiscard( "Const getter" )]]
    auto is_initialized( ) const -> bool;

    auto new_frame( ) -> void;

    auto render( vk::CommandBuffer const& command_buffer ) const -> void;

private:
    window::GlfwWindow&            glfw_window_;
    vlk::objs::VulkanGpu&          gpu_;
    vlk::objs::VulkanPresentation& presentation_;
    DpiFontHandling                dpi_handling_;

    /// \brief RAII object to handle ImGui setup and destruction.
    ImguiContextHandle imgui_context_ = nullptr;

    /// \brief RAII object to handle ImGui GLFW setup and destruction.
    ImguiGlfwHandle imgui_glfw_ = nullptr;

    /// \brief RAII object to handle ImGui Vulkan setup and destruction.
    ImguiVulkanHandle imgui_vulkan_ = nullptr;

    uint min_image_count_ = 0U;
};

} // namespace ltb::gui
