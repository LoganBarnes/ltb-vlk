// ///////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022 Logan Barnes - All Rights Reserved
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/utils/result.hpp"
#include "ltb/utils/type_string.hpp"

// standard
#include <algorithm>
#include <list>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ltb::utils
{

template < typename Map, typename Key >
auto has_key( Map const& map, Key const& key ) -> bool
{
    return map.find( key ) != map.end( );
}

template < typename V, typename T >
auto has_item( V& v, T const& item )
{
    return std::find( std::begin( v ), std::end( v ), item ) != std::end( v );
}

template < typename V, typename P >
auto has_item_if( V& v, P const& pred )
{
    return std::find_if( std::begin( v ), std::end( v ), pred ) != std::end( v );
}

template < typename V, typename T >
auto remove_all_by_key( V& v, T const& key )
{
    return v.erase( key );
}

template < typename V, typename T >
auto remove_all_by_value( V& v, T const& value )
{
    return v.erase( std::remove( std::begin( v ), std::end( v ), value ), std::end( v ) );
}

template < typename V, typename Pred >
auto remove_all_by_predicate( V& v, Pred&& pred )
{
    return v.erase(
        std::remove_if( std::begin( v ), std::end( v ), std::forward< Pred >( pred ) ),
        std::end( v )
    );
}

template < typename V, typename Pred >
auto iterate_with_removals( V& v, Pred&& pred )
{
    for ( auto iter = v.begin( ); iter != v.end( ); /*no-op*/ )
    {
        if ( pred( *iter ) )
        {
            iter = v.erase( iter );
        }
        else
        {
            ++iter;
        }
    }
}

template <
    typename Container1,
    typename Container2,
    typename Value = std::decay_t< decltype( std::declval< Container1 >( ).front( ) ) > >
auto find_first_available( Container1 const& preferred_values, Container2 const& available_values )
    -> Result< Value >
{
    if ( available_values.empty( ) )
    {
        return LTB_MAKE_UNEXPECTED_ERROR( "No available {} values", type_string< Value >( ) );
    }

    for ( auto const& preferred_value : preferred_values )
    {
        if ( std::find( available_values.begin( ), available_values.end( ), preferred_value )
             != available_values.end( ) )
        {
            return preferred_value;
        }
    }

    return LTB_MAKE_UNEXPECTED_ERROR(
        "None of the preferred {} values available.",
        type_string< Value >( )
    );
}

template < typename Data, typename Object >
using MemberFunction = Data ( Object::* )( ) const;

/// \brief Extract a member function value from a class/struct.
template < typename Data, typename Object >
class GetMemberFunction
{
public:
    explicit GetMemberFunction( MemberFunction< Data, Object > const member_func )
        : member_function_( member_func )
    {
    }

    auto operator( )( Object const& obj ) -> Data
    {
        return ( obj.*member_function_ )( );
    }

private:
    MemberFunction< Data, Object > member_function_;
};

} // namespace ltb::utils
