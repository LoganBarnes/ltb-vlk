// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/geom/range.hpp"
#include "ltb/gui/imgui.hpp"
#include "ltb/utils/error.hpp"

// external
#include <magic_enum.hpp>

// standard
#include <algorithm>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace ltb::gui
{

/// \brief The bounds of current window excluding any title or menu bars.
auto get_window_content_bounds( ) -> geom::Range2;

/// \brief Calls ImGui::PushID on construction, PopID on destruction.
class ScopedId
{
public:
    // ImGui::PushID on construction
    template < typename T >
    explicit ScopedId( T const* ptr_id );
    explicit ScopedId( char const* str_id );
    explicit ScopedId( char const* str_id_begin, char const* str_id_end );
    explicit ScopedId( int int_id );

    // ImGui::PopID on destruction
    ~ScopedId( );

    // No copy
    ScopedId( ScopedId const& )                    = delete;
    auto operator=( ScopedId const& ) -> ScopedId& = delete;

    // No move
    ScopedId( ScopedId&& ) noexcept                    = delete;
    auto operator=( ScopedId&& ) noexcept -> ScopedId& = delete;
};

template < typename T >
ScopedId::ScopedId( T const* const ptr_id )
{
    ImGui::PushID( ptr_id );
}

class ScopedDisable
{
public:
    // ImGui::BeginDisabled on construction
    explicit ScopedDisable( bool disable );

    // ImGui::EndDisabled on destruction
    ~ScopedDisable( );

    // No copy
    ScopedDisable( ScopedDisable const& )                    = delete;
    auto operator=( ScopedDisable const& ) -> ScopedDisable& = delete;

    // No move
    ScopedDisable( ScopedDisable&& ) noexcept                    = delete;
    auto operator=( ScopedDisable&& ) noexcept -> ScopedDisable& = delete;
};

enum class Alignment
{
    Vertical,
    SameLine,
};

/// \brief Create a list of radio buttons used to select an enum value
/// \tparam Enum - The type of enum
/// \param label - The top-level label for these enum types
/// \param current_value - the value that will be modified when the gui is clicked
/// \param alignment - how to align the enums in the GUI
/// \param labels_and_values - this will be auto generated if it is empty
/// \return true if the enum value changed
template < typename Enum >
auto enum_radio_buttons(
    std::string const&                            label,
    Enum*                                         current_value,
    Alignment                                     alignment         = Alignment::Vertical,
    std::vector< std::pair< std::string, Enum > > labels_and_values = { }
) -> bool
{
    static_assert( std::is_enum_v< Enum >, "enum_radio_buttons input must be an enum type" );
    auto const id = ScopedId{ current_value };

    if ( labels_and_values.empty( ) )
    {
        // auto populate:
        for ( auto value : magic_enum::enum_values< Enum >( ) )
        {
            labels_and_values.emplace_back( magic_enum::enum_name( value ), value );
        }
    }

    if ( !label.empty( ) || alignment == Alignment::SameLine )
    {
        ImGui::Text( "%s", label.c_str( ) );
    }

    auto something_changed = false;

    for ( auto const& [ button_label, value ] : labels_and_values )
    {
        if ( alignment == Alignment::SameLine )
        {
            ImGui::SameLine( );
        }
        if ( ImGui::RadioButton( button_label.c_str( ), *current_value == value ) )
        {
            *current_value    = value;
            something_changed = true;
        }
    }
    return something_changed;
}

/// \brief Create a dropdown menu used to select a value
/// \param label - The top-level label for these types
/// \param current_value - the value that will be modified when the gui is clicked
/// \param labels_and_values - the list of labels to display and the values they represent
/// \return true if the value is changed
template < typename T >
auto configure_combo_menu(
    std::string const&                                label,
    T*                                                current_value,
    std::vector< std::pair< std::string, T > > const& labels_and_values
) -> bool
{

    auto iter = std::find_if(
        labels_and_values.begin( ),
        labels_and_values.end( ),
        [ current_value ]( auto& name_and_enum ) { return name_and_enum.second == *current_value; }
    );

    auto display_str = ( iter != labels_and_values.end( ) ? iter->first : std::string{ } );

    auto value_updated = false;

    if ( ImGui::BeginCombo( label.c_str( ), display_str.c_str( ) ) )
    {
        for ( auto const& [ value_label, value ] : labels_and_values )
        {
            if ( ImGui::Selectable( value_label.c_str( ), value_label == display_str ) )
            {
                *current_value = value;
                value_updated  = true;
            }
        }
        ImGui::EndCombo( );
    }

    return value_updated;
}

enum class Sort
{
    No,
    Yes
};

/// \brief Create a dropdown menu used to select an enum value
/// \tparam Enum - The type of enum
/// \param label - The top-level label for these enum types
/// \param current_value - the value that will be modified when the gui is clicked
/// \param labels_and_values - this will be auto generated if it is empty
/// \return true if the enum value changed
template < typename Enum >
auto enum_combo_menu(
    std::string const&                            label,
    Enum*                                         current_value,
    std::vector< std::pair< std::string, Enum > > labels_and_values = { },
    Sort                                          sort              = Sort::No
) -> bool
{
    static_assert( std::is_enum_v< Enum >, "enum_combo_menu input must be an enum type" );

    if ( labels_and_values.empty( ) )
    {
        // auto populate:
        for ( auto value : magic_enum::enum_values< Enum >( ) )
        {
            labels_and_values.emplace_back( magic_enum::enum_name( value ), value );
        }
    }

    if ( sort == Sort::Yes )
    {
        std::sort(
            labels_and_values.begin( ),
            labels_and_values.end( ),
            []( auto const& lhs, auto const& rhs ) { return lhs.first < rhs.first; }
        );
    }

    return configure_combo_menu( label, current_value, labels_and_values );
}

/// \brief This is the function type for the ImGui text display functions (ImGui::Text,
/// ImGui::TextV, etc.).
/// \details These ImGui text functions support C-style printf formating, but often it is
///          more useful to use the fmtlib style of formatting. The gui::imgui_fmt* functions
///          wrap the C-style ImGui functions to allow for fmtlib style formatting.
using ImGuiTextFunction = void ( * )( char const* text, ... );

/// \brief Format a string before passing it to a C-style text formatting function.
/// \tparam TextCallable - The ImGui text display function to call (ImGui::Text, ImGui::SetTooltip,
/// etc.)
template < ImGuiTextFunction TextCallable, typename... Args >
auto imgui_fmt( fmt::format_string< Args... > fmt, Args&&... args )
{
    auto str = fmt::format( fmt, std::forward< Args >( args )... );
    return TextCallable( "%s", str.c_str( ) );
}

/// \brief Format a string before passing it to an ImGui::TextColored text based function.
template < typename Color, typename... Args >
auto imgui_fmt_colored( Color color, fmt::format_string< Args... > fmt, Args&&... args )
{
    auto str = fmt::format( fmt, std::forward< Args >( args )... );
    return ImGui::TextColored( color, "%s", str.c_str( ) );
}

/// \brief If the user is hovering over the previous item, format a
///        string and pass it to a C-style text formatting function.
template < ImGuiTextFunction TextCallable, typename... Args >
auto imgui_fmt_hovered( fmt::format_string< Args... > fmt, Args&&... args ) -> void
{
    if ( ImGui::IsItemHovered( ) )
    {
        return imgui_fmt< TextCallable >( fmt, std::forward< Args >( args )... );
    }
}

/// \brief If the user is hovering over the previous item, format a
///        string and pass it to ImGui::SetTooltip.
template < typename... Args >
auto imgui_tooltip( fmt::format_string< Args... > fmt, Args&&... args ) -> void
{
    imgui_fmt_hovered< ImGui::SetTooltip >( fmt, std::forward< Args >( args )... );
}

} // namespace ltb::gui
