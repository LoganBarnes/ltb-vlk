// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/utils/result.hpp"

// standard
#include <concepts>

namespace ltb::utils
{

template < typename T >
concept Initializable = requires( T& a ) {
    { a.initialize( ) } -> std::same_as< Result< void > >;
    { a.is_initialized( ) } -> std::same_as< bool >;
};

/// \brief Initializes `object` unless it already is.
struct Initialize
{
    template < typename Object >
        requires Initializable< Object >
    auto operator( )( Object& object ) const -> Result< void >
    {
        if ( object.is_initialized( ) )
        {
            return success( );
        }
        return object.initialize( );
    }
};

struct IsInitialized
{
    template < typename Object >
        requires Initializable< Object >
    auto operator( )( Object const& object ) const -> bool
    {
        return object.is_initialized( );
    }
};

} // namespace ltb::utils
