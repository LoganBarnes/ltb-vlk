// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/gui/imgui.hpp"
#include "ltb/utils/result.hpp"

namespace ltb::gui
{

struct ImguiContextDeleter
{
    auto operator( )( ImGuiContext* context ) const -> void;
};

using ImguiContextHandle = std::unique_ptr< ImGuiContext, ImguiContextDeleter >;

auto initialize_imgui_context( ) -> utils::Result< ImguiContextHandle >;

} // namespace ltb::gui
