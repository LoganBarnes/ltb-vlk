// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/gui/imgui_vulkan.hpp"

// project
#include "ltb/vlk/objs/vulkan_gpu.hpp"
#include "ltb/vlk/objs/vulkan_presentation.hpp"

// external
#include <imgui_impl_vulkan.h>

namespace ltb::gui
{

ScopedImguiVulkan::ScopedImguiVulkan( ::ImGui_ImplVulkan_InitInfo& init_info )
    : init_value{ ::ImGui_ImplVulkan_Init( &init_info ) }
{
}

ScopedImguiVulkan::~ScopedImguiVulkan( )
{
    if ( init_value )
    {
        ::ImGui_ImplVulkan_Shutdown( );
    }
}

auto initialize_imgui_vulkan(
    vlk::objs::VulkanGpu&          gpu,
    vlk::objs::VulkanPresentation& presentation
) -> utils::Result< ImguiVulkanHandle >
{
    LTB_CHECK_VALID( gpu.is_initialized( ) );
    LTB_CHECK_VALID( presentation.is_initialized( ) );

    constexpr auto queue_type = vlk::QueueType::Graphics;
    LTB_CHECK_VALID( gpu.physical_device( ).queue_families( ).contains( queue_type ) );
    LTB_CHECK_VALID( gpu.device( ).queues( ).contains( queue_type ) );

    auto init_info = ::ImGui_ImplVulkan_InitInfo{ };

    init_info.ApiVersion     = gpu.instance( ).app_info( ).apiVersion;
    init_info.Instance       = gpu.instance( ).get( );
    init_info.PhysicalDevice = gpu.physical_device( ).get( );
    init_info.Device         = gpu.device( ).get( );
    init_info.QueueFamily    = gpu.physical_device( ).queue_families( ).at( queue_type );
    init_info.Queue          = gpu.device( ).queues( ).at( queue_type );
    init_info.DescriptorPool = gpu.descriptor_pool( ).get( );
    init_info.MinImageCount  = presentation.swapchain( ).min_image_count( );
    init_info.ImageCount = static_cast< uint32 >( presentation.swapchain_image_views( ).size( ) );

    init_info.PipelineInfoMain.RenderPass = presentation.render_pass( ).get( );
    init_info.PipelineInfoMain.Subpass    = 0U;

    if ( auto imgui_vulkan = std::make_unique< ScopedImguiVulkan >( init_info );
         imgui_vulkan->init_value )
    {
        return imgui_vulkan;
    }
    return LTB_MAKE_UNEXPECTED_ERROR( "ImGui_ImplVulkan_Init() failed" );
}

} // namespace ltb::gui
