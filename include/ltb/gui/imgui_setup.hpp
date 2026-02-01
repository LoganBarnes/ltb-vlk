// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/utils/result.hpp"
#include "ltb/vlk/vulkan.hpp"

namespace ltb::gui
{

class ImguiSetup
{
public:
    virtual ~ImguiSetup( ) = 0;

    /// \brief Initialize ImGui with the default context.
    virtual auto initialize( ) -> utils::Result< void > = 0;

    /// \brief Check if ImGui is initialized.
    [[nodiscard( "Const getter" )]]
    virtual auto is_initialized( ) const -> bool = 0;

    /// \brief Start a new ImGui frame.
    virtual auto new_frame( ) -> void = 0;

    /// \brief Render the ImGui draw data.
    virtual auto render( vk::CommandBuffer const& command_buffer ) const -> void = 0;
};

inline ImguiSetup::~ImguiSetup( ) = default;

} // namespace ltb::gui
