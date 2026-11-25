// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/utils/enum_flags.hpp"

// external
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

// standard
#include <filesystem>
#include <fstream>
#include <iomanip>

namespace ltb::utils
{

enum class JsonSettingsFlag
{
    NoImplicitLoad,
    NoImplicitSave,
    PrintDebugMessages,
};

/// \brief Loads and saves settings from and to a json file.
template < typename T >
class JsonSettings
{
public:
    template < typename... Args >
    explicit JsonSettings(
        std::filesystem::path            file,
        std::string                      name,
        utils::Flags< JsonSettingsFlag > flags,
        Args&&... args
    );
    explicit JsonSettings( std::filesystem::path file, std::string name );
    ~JsonSettings( );

    // No copy
    JsonSettings( JsonSettings const& )                    = delete;
    auto operator=( JsonSettings const& ) -> JsonSettings& = delete;

    // Move only
    JsonSettings( JsonSettings&& ) noexcept                    = default;
    auto operator=( JsonSettings&& ) noexcept -> JsonSettings& = default;

    auto operator->( ) const -> T const*;
    auto operator->( ) -> T*;
    auto operator*( ) const -> T const&;
    auto operator*( ) -> T&;

    T value = { };

    /// \brief Loads the JSON settings from disk, if the path exist. Leaves `value`
    ///        unchanged (and logs a warning) if the file isn't valid JSON or the settings
    ///        don't match `T`.
    auto load_settings( std::filesystem::path optional_path_override = { } ) -> void;

    /// \brief Saves the settings to disk as JSON, keeping any other entries in the file.
    ///        Does not overwrite (and logs a warning) if the existing file isn't a valid
    ///        JSON object, so a hand-edited file with a typo isn't lost.
    auto save_settings( std::filesystem::path optional_path_override = { } ) -> void;

private:
    std::filesystem::path            file_;
    std::string                      name_;
    utils::Flags< JsonSettingsFlag > flags_;

    // Shared pointers use reference counters so we use an
    // empty one to handle reference counting for us.
    std::shared_ptr< void > deletion_counter_;
};

template < typename T >
template < typename... Args >
JsonSettings< T >::JsonSettings(
    std::filesystem::path            file,
    std::string                      name,
    utils::Flags< JsonSettingsFlag > flags,
    Args&&... args
)
    : value{ std::forward< Args >( args )... }
    , file_( std::move( file ) )
    , name_( std::move( name ) )
    , flags_( flags )
    , deletion_counter_( &value, []( auto const* ) { /*no-op*/ } )
{

    // Load on construction
    if ( !has_flag( flags_, JsonSettingsFlag::NoImplicitLoad ) )
    {
        load_settings( );
    }
}

template < typename T >
JsonSettings< T >::JsonSettings( std::filesystem::path file, std::string name )
    : JsonSettings<
          T >( std::move( file ), std::move( name ), utils::no_flags< JsonSettingsFlag >( ) )
{
}

template < typename T >
JsonSettings< T >::~JsonSettings( )
{
    if ( deletion_counter_.use_count( ) == 1 )
    {
        // Save on destruction. Destructors must not throw, so log any failure instead.
        if ( !has_flag( flags_, JsonSettingsFlag::NoImplicitSave ) )
        {
            try
            {
                save_settings( );
            }
            catch ( std::exception const& e )
            {
                spdlog::error(
                    "Failed to save JSON '{}' to '{}': {}",
                    name_,
                    file_.string( ),
                    e.what( )
                );
            }
        }
    }
}

template < typename T >
auto JsonSettings< T >::operator->( ) const -> T const*
{
    return &value;
}

template < typename T >
auto JsonSettings< T >::operator->( ) -> T*
{
    return &value;
}

template < typename T >
auto JsonSettings< T >::operator*( ) const -> T const&
{
    return value;
}

template < typename T >
auto JsonSettings< T >::operator*( ) -> T&
{
    return value;
}

template < typename T >
auto JsonSettings< T >::load_settings( std::filesystem::path optional_path_override ) -> void
{
    auto path = optional_path_override;
    if ( optional_path_override.empty( ) )
    {
        path = file_;
    }

    if ( std::filesystem::exists( path ) )
    {
        auto file = std::ifstream( path );

        constexpr auto no_callback      = nullptr;
        constexpr auto allow_exceptions = false;
        auto const     json = nlohmann::json::parse( file, no_callback, allow_exceptions );

        if ( json.is_discarded( ) )
        {
            spdlog::warn( "JSON '{}' not loaded. Invalid JSON in '{}'", name_, path.string( ) );
            return;
        }

        if ( json.is_object( ) && json.contains( name_ ) )
        {
            // Parse into a copy so a partial failure doesn't leave `value` half-updated.
            try
            {
                auto loaded = value;
                json.at( name_ ).get_to( loaded );
                value = std::move( loaded );
            }
            catch ( nlohmann::json::exception const& e )
            {
                spdlog::
                    warn( "JSON '{}' not loaded from '{}': {}", name_, path.string( ), e.what( ) );
                return;
            }
        }

        if ( has_flag( flags_, JsonSettingsFlag::PrintDebugMessages ) )
        {
            spdlog::debug( "Loaded JSON '{}' from '{}'", name_, path.string( ) );
        }
    }
    else if ( has_flag( flags_, JsonSettingsFlag::PrintDebugMessages ) )
    {
        spdlog::debug( "JSON '{}' not loaded. File does not exist: '{}'", name_, path.string( ) );
    }
}

template < typename T >
auto JsonSettings< T >::save_settings( std::filesystem::path optional_path_override ) -> void
{
    auto path = optional_path_override;
    if ( optional_path_override.empty( ) )
    {
        path = file_;
    }

    auto json = nlohmann::json::object( );

    if ( std::filesystem::exists( path ) )
    {
        // Keep any other settings stored in the same file.
        auto file = std::ifstream( path );

        constexpr auto no_callback      = nullptr;
        constexpr auto allow_exceptions = false;
        json = nlohmann::json::parse( file, no_callback, allow_exceptions );

        if ( json.is_discarded( ) || !json.is_object( ) )
        {
            spdlog::warn(
                "JSON '{}' not saved. '{}' exists but is not a JSON object",
                name_,
                path.string( )
            );
            return;
        }
    }

    json[ name_ ] = value;

    // write prettified JSON to file
    auto file = std::ofstream( path );
    file << std::setw( 2 ) << json << std::endl;

    if ( has_flag( flags_, JsonSettingsFlag::PrintDebugMessages ) )
    {
        spdlog::debug( "Saved JSON '{}' to '{}'", name_, path.string( ) );
    }
}

} // namespace ltb::utils
