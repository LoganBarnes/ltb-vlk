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

struct Initialize
{
    template < typename Object >
        requires Initializable< Object >
    auto operator( )( Object const& object ) const -> Result< void >
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

// struct FailedResult
// {
//     Result< void > result;
//
//     template < typename Object >
//         requires Initializable< Object >
//     auto operator( )( Object& object ) -> bool
//     {
//         result = success( );
//         if ( !object.is_initialized( ) )
//         {
//             result = object.initialize( );
//         }
//         return !result;
//     }
// };
//
// /// \brief Initializes a list of objects that implement the Initializable concept.
// template < typename... Objs >
// auto initialize( Objs&... objs ) -> Result< void >
// {
//     if ( auto failed_result = FailedResult{ }; ( failed_result( objs ) || ... ) )
//     {
//         return failed_result.result;
//     }
//     return success( );
// }
//
// /// \brief Checks if all objects that implement the Initializable concept are initialized.
// template < typename... Objs >
//     requires( Initializable< Objs > && ... )
// auto is_initialized( Objs const&... objs ) -> bool
// {
//     return ( objs.is_initialized( ) && ... );
// }

} // namespace ltb::utils
