// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/utils/result.hpp"
#include "ltb/vlk/fwd.hpp"
#include "ltb/vlk/objs/fwd.hpp"

struct ImGui_ImplVulkan_InitInfo;

namespace ltb::gui
{

class ScopedImguiVulkan
{
public:
    explicit ScopedImguiVulkan( ::ImGui_ImplVulkan_InitInfo& init_info );
    ~ScopedImguiVulkan( );

    bool init_value;
};

using ImguiVulkanHandle = std::unique_ptr< ScopedImguiVulkan >;

auto initialize_imgui_vulkan(
    vlk::objs::VulkanGpu&          gpu,
    vlk::objs::VulkanPresentation& presentation
) -> utils::Result< ImguiVulkanHandle >;

} // namespace ltb::gui
