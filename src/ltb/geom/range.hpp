// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/geom/fwd.hpp"

namespace ltb::geom
{

template < typename T >
struct Range
{
    T min = { };
    T max = { };
};

template < typename T >
constexpr auto dimensions( Range< T > const& range )
{
    return range.max - range.min;
}

template < typename T >
constexpr auto center( Range< T > const& range )
{
    // Find the center in a way that avoids overflow.
    constexpr auto two = T( 2 );
    return ( range.min / two ) + ( range.max / two );
}

template < glm::length_t L, typename T >
auto contains( Range< glm::vec< L, T > > const& range, glm::vec< L, T > const& p )
{
    for ( auto i = 0; i < L; ++i )
    {
        if ( ( range.min[ i ] > p[ i ] ) || ( p[ i ] > range.max[ i ] ) )
        {
            return false;
        }
    }
    return true;
}

template < typename T >
constexpr auto contains( Range< T > const& range, T v ) -> bool
{
    return ( range.min <= v ) && ( v <= range.max );
}

} // namespace ltb::geom
