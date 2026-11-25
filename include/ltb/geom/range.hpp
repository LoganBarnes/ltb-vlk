// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/geom/fwd.hpp"

// standard
#include <numeric>
#include <type_traits>

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
    if constexpr ( std::is_arithmetic_v< T > )
    {
        // Avoids overflow, and rounds towards `min` for integers.
        return std::midpoint( range.min, range.max );
    }
    else
    {
        // Vector types (e.g. glm::vec2, glm::ivec2).
        return range.min + ( ( range.max - range.min ) / typename T::value_type( 2 ) );
    }
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
