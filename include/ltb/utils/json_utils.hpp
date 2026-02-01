// ///////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022 Rotor Technologies, Inc. - All Rights Reserved
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// external
#include <nlohmann/json.hpp>

// standard
#include <filesystem>
#include <variant>

namespace ltb::utils
{

template < typename... Args >
auto variant_to_json( nlohmann::json& json, std::variant< Args... > const& op ) -> void
{
    std::visit( [ &json ]( auto const& value ) { json = value; }, op );
}

template < typename... Args >
auto variant_from_json( nlohmann::json const& json, std::variant< Args... >& op ) -> void
{
    std::visit( [ &json ]( auto& value ) { json.get_to( value ); }, op );
}

template < typename T, typename... Args >
auto variant_from_json_as( nlohmann::json const& json, std::variant< Args... >& op ) -> void
{
    op = json.get< T >( );
}

template < typename T >
auto assign_if_present( nlohmann::json const& json, std::string const& key, T& value ) -> bool
{
    if ( json.contains( key ) )
    {
        json.at( key ).get_to( value );
        return true;
    }
    return false;
}

/// \brief Convert to a JSON object and back. Useful for testing.
/// \returns a new T extracted from the JSON object.
template < typename T >
auto convert_to_json_and_back( T const& original ) -> T
{
    nlohmann::json json = original;
    return json.get< T >( );
}

} // namespace ltb::utils

namespace nlohmann
{

template <>
struct adl_serializer< std::filesystem::path >
{
    static void to_json( json& j, std::filesystem::path const& path )
    {
        j = path.string( );
    }

    static void from_json( json const& j, std::filesystem::path& path )
    {
        path = j.get< std::string >( );
    }
};

} // namespace nlohmann
