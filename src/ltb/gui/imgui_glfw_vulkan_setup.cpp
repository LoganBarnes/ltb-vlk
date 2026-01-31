// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/gui/imgui_glfw_vulkan_setup.hpp"

// project
#include "ltb/vlk/objs/vulkan_gpu.hpp"
#include "ltb/vlk/objs/vulkan_presentation.hpp"
#include "ltb/window/glfw_window.hpp"

// external
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

namespace ltb::gui
{

ImguiGlfwVulkanSetup::ImguiGlfwVulkanSetup(
    window::GlfwWindow&            glfw_window,
    vlk::objs::VulkanGpu&          gpu,
    vlk::objs::VulkanPresentation& presentation
)
    : ImguiGlfwVulkanSetup( glfw_window, gpu, presentation, DpiFontHandling::Scale )
{
}

ImguiGlfwVulkanSetup::ImguiGlfwVulkanSetup(
    window::GlfwWindow&            glfw_window,
    vlk::objs::VulkanGpu&          gpu,
    vlk::objs::VulkanPresentation& presentation,
    DpiFontHandling                dpi_handling
)
    : glfw_window_( glfw_window )
    , gpu_( gpu )
    , presentation_( presentation )
    , dpi_handling_( dpi_handling )
{
}

auto ImguiGlfwVulkanSetup::initialize( ) -> utils::Result< void >
{
    if ( is_initialized( ) )
    {
        return utils::success( );
    }
    LTB_CHECK_VALID( gpu_.is_initialized( ) );
    LTB_CHECK_VALID( presentation_.is_initialized( ) );

    LTB_CHECK( auto imgui_context, gui::initialize_imgui_context( ) );
    LTB_CHECK( auto imgui_glfw, gui::initialize_imgui_glfw( glfw_window_.get( ) ) );
    LTB_CHECK( auto imgui_vulkan, gui::initialize_imgui_vulkan( gpu_, presentation_ ) );

    auto& io = ImGui::GetIO( );

    io.ConfigFlags |= ::ImGuiConfigFlags_DockingEnable;

    if ( dpi_handling_ == DpiFontHandling::Scale )
    {
        // If the user wants to scale the fonts based on the primary monitor's DPI,
        // we need to get the content scale of the primary monitor.
        // This is necessary for high-DPI screens where the default font size may be too small.
        auto primary_monitor_scale = glm::vec2{ 1.0F, 1.0F };
        if ( GLFWmonitor* monitor = ::glfwGetPrimaryMonitor( ) )
        {
            ::glfwGetMonitorContentScale(
                monitor,
                &primary_monitor_scale.x,
                &primary_monitor_scale.y
            );
        }
    }

    // No more errors will happen. Store all the initialized objects.
    // If the function fails before this point, the temporary objects will
    // be deleted and all members of the class will remain uninitialized.
    imgui_context_   = std::move( imgui_context );
    imgui_glfw_      = std::move( imgui_glfw );
    imgui_vulkan_    = std::move( imgui_vulkan );
    min_image_count_ = presentation_.swapchain( ).min_image_count( );

    return utils::success( );
}

auto ImguiGlfwVulkanSetup::is_initialized( ) const -> bool
{
    return ( nullptr != imgui_context_ ) && ( nullptr != imgui_glfw_ )
        && ( nullptr != imgui_vulkan_ );
}

auto ImguiGlfwVulkanSetup::new_frame( ) -> void
{
    if ( presentation_.swapchain( ).min_image_count( ) != min_image_count_ )
    {
        min_image_count_ = presentation_.swapchain( ).min_image_count( );
        ::ImGui_ImplVulkan_SetMinImageCount( min_image_count_ );
    }
    ::ImGui_ImplVulkan_NewFrame( );
    ::ImGui_ImplGlfw_NewFrame( );
    ImGui::NewFrame( );
}

auto ImguiGlfwVulkanSetup::render( vk::CommandBuffer const& command_buffer ) const -> void
{
    ImGui::Render( );
    ::ImGui_ImplVulkan_RenderDrawData( ImGui::GetDrawData( ), command_buffer );
}

} // namespace ltb::gui
