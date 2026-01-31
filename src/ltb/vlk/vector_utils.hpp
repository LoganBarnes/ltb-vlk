// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/utils/container_utils.hpp"
#include "ltb/utils/result.hpp"

// external
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/transform.hpp>

namespace ltb::vlk
{

struct Get
{
    auto operator( )( auto const& object ) const
    {
        return object.get( );
    }

    auto operator( )( auto& object ) const
    {
        return object.get( );
    }
};

template < typename Object, typename ObjectSettings, typename... Args >
auto initialize_all( std::vector< ObjectSettings > object_settings, Args&... args )
    -> utils::Result< std::vector< Object > >
{
    auto objects = std::vector< Object >{ };
    objects.reserve( object_settings.size( ) );

    for ( auto& settings : object_settings )
    {
        LTB_CHECK( objects.emplace_back( args... ).initialize( std::move( settings ) ) );
    }

    return objects;
}

template < typename Object, typename Data >
auto get_member_functions(
    std::vector< Object > const&                objects,
    utils::MemberFunction< Data, Object > const member_function
) -> std::vector< std::decay_t< Data > >
{
    return objects
         | ranges::views::transform( utils::GetMemberFunction< Data, Object >{ member_function } )
         | ranges::to< std::vector >( );
}

template < typename Object, typename Data >
auto get_member_functions(
    std::vector< std::reference_wrapper< Object > > const& objects,
    utils::MemberFunction< Data, Object > const            member_function
) -> std::vector< std::decay_t< Data > >
{
    return objects
         | ranges::views::transform( utils::GetMemberFunction< Data, Object >{ member_function } )
         | ranges::to< std::vector >( );
}

} // namespace ltb::vlk
