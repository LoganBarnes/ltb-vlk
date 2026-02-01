// ///////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022 Logan Barnes - All Rights Reserved
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// external
#include <magic_enum.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/filter.hpp>

// standard
#include <limits>
#include <type_traits>

namespace ltb::utils
{

template < typename E, std::enable_if_t< std::is_enum_v< E >, int > = 0 >
constexpr auto to_underlying( E const& value )
{
    return static_cast< std::underlying_type_t< E > >( value );
}

template < typename E, std::enable_if_t< std::is_enum_v< E >, int > = 0 >
constexpr auto from_underlying( std::underlying_type_t< E > const& value ) -> E
{
    return static_cast< E >( value );
}

/// \brief Converts incremental enums to bit values
///
/// \code
/// enum class Enum : int {
///     first,
///     second,
///     third
/// }
///
/// to_bits(Enum::first);  // -> 0b0001
/// to_bits(Enum::second); // -> 0b0010
/// to_bits(Enum::third);  // -> 0b0100
/// \endcode
template < typename E, std::enable_if_t< std::is_enum_v< E >, int > = 0 >
constexpr auto to_bits( E type ) -> std::underlying_type_t< E >
{
    return std::underlying_type_t< E >( 1 ) << to_underlying( type );
}

template < typename E >
struct Flags
{
    using Type = std::underlying_type_t< E >;

    static_assert( std::is_enum_v< E > );
    static_assert( magic_enum::enum_count< E >( ) <= std::numeric_limits< Type >::digits );

    Flags( ) = default;

    // NOLINTNEXTLINE(google-explicit-constructor)
    Flags( E initial_flag )
        : bits( to_bits( initial_flag ) )
    {
    }

    explicit Flags( Type initial_bits )
        : bits( initial_bits )
    {
    }

    operator bool( ) const
    {
        return bits != 0;
    } // NOLINT(google-explicit-constructor)

    Type bits = Type( 0 );
};

template < typename E >
auto operator==( Flags< E > lhs, Flags< E > rhs ) -> bool
{
    return lhs.bits == rhs.bits;
}

template < typename E >
auto operator!=( Flags< E > lhs, Flags< E > rhs ) -> bool
{
    return !( lhs == rhs );
}

namespace flag_operators
{

// ~
template < typename E >
constexpr auto operator~( Flags< E > flags ) noexcept -> Flags< E >
{
    return Flags< E >{ static_cast< typename Flags< E >::Type >( ~flags.bits ) };
}

template < typename E >
constexpr auto operator~( E value ) noexcept -> Flags< E >
{
    return ~Flags< E >{ value };
}

// |
template < typename E >
constexpr auto operator|( Flags< E > lhs, Flags< E > rhs ) noexcept -> Flags< E >
{
    return Flags< E >{ static_cast< typename Flags< E >::Type >( lhs.bits | rhs.bits ) };
}

template < typename E >
constexpr auto operator|( Flags< E > lhs, E rhs ) noexcept -> Flags< E >
{
    return ( lhs | Flags< E >{ rhs } );
}

template < typename E >
constexpr auto operator|( E lhs, Flags< E > rhs ) noexcept -> Flags< E >
{
    return ( Flags< E >{ lhs } | rhs );
}

template < typename E >
constexpr auto operator|( E lhs, E rhs ) noexcept -> Flags< E >
{
    return ( Flags< E >{ lhs } | Flags< E >{ rhs } );
}

// &
template < typename E >
constexpr auto operator&( Flags< E > lhs, Flags< E > rhs ) noexcept -> Flags< E >
{
    return Flags< E >{ static_cast< typename Flags< E >::Type >( lhs.bits & rhs.bits ) };
}

template < typename E >
constexpr auto operator&( Flags< E > lhs, E rhs ) noexcept -> Flags< E >
{
    return ( lhs & Flags< E >{ rhs } );
}

template < typename E >
constexpr auto operator&( E lhs, Flags< E > rhs ) noexcept -> Flags< E >
{
    return ( Flags< E >{ lhs } & rhs );
}

template < typename E >
constexpr auto operator&( E lhs, E rhs ) noexcept -> Flags< E >
{
    return ( Flags< E >{ lhs } & Flags< E >{ rhs } );
}

// ^
template < typename E >
constexpr auto operator^( Flags< E > lhs, Flags< E > rhs ) noexcept -> Flags< E >
{
    return Flags< E >{ static_cast< typename Flags< E >::Type >( lhs.bits ^ rhs.bits ) };
}

template < typename E >
constexpr auto operator^( Flags< E > lhs, E rhs ) noexcept -> Flags< E >
{
    return ( lhs ^ Flags< E >{ rhs } );
}

template < typename E >
constexpr auto operator^( E lhs, Flags< E > rhs ) noexcept -> Flags< E >
{
    return ( Flags< E >{ lhs } ^ rhs );
}

template < typename E >
constexpr auto operator^( E lhs, E rhs ) noexcept -> Flags< E >
{
    return ( Flags< E >{ lhs } ^ Flags< E >{ rhs } );
}

// |=
template < typename E >
constexpr auto operator|=( Flags< E >& lhs, Flags< E > rhs ) noexcept -> Flags< E >&
{
    return lhs = ( lhs | rhs );
}

template < typename E >
constexpr auto operator|=( Flags< E >& lhs, E rhs ) noexcept -> Flags< E >&
{
    return lhs = ( lhs | Flags< E >{ rhs } );
}

// &=
template < typename E >
constexpr auto operator&=( Flags< E >& lhs, Flags< E > rhs ) noexcept -> Flags< E >&
{
    return lhs = ( lhs & rhs );
}

template < typename E >
constexpr auto operator&=( Flags< E >& lhs, E rhs ) noexcept -> Flags< E >&
{
    return lhs = ( lhs & Flags< E >{ rhs } );
}

// ^=
template < typename E >
constexpr auto operator^=( Flags< E >& lhs, Flags< E > rhs ) noexcept -> Flags< E >&
{
    return lhs = ( lhs ^ rhs );
}

template < typename E >
constexpr auto operator^=( Flags< E >& lhs, E rhs ) noexcept -> Flags< E >&
{
    return lhs = ( lhs ^ Flags< E >{ rhs } );
}

} // namespace flag_operators

template < typename E, typename... Es >
constexpr auto make_flags( E flag ) -> Flags< E >
{
    using namespace flag_operators;
    return Flags< E >{ flag };
}

template < typename E, typename... Es >
constexpr auto make_flags( E flag, Es... flags ) -> Flags< E >
{
    using namespace flag_operators;
    return make_flags< E >( flag ) | make_flags< E >( std::forward< Es >( flags )... );
}

template < typename E >
constexpr auto add_flag( Flags< E > flags, E flag ) -> Flags< E >
{
    using namespace flag_operators;
    return flags | flag;
}

template < typename E >
constexpr auto remove_flag( Flags< E > flags, E flag ) -> Flags< E >
{
    using namespace flag_operators;
    return flags & ~flag;
}

template < typename E >
constexpr auto toggle_flag( Flags< E > flags, E flag ) -> Flags< E >
{
    using namespace flag_operators;
    return flags ^ flag;
}

template < typename E >
constexpr auto has_flag( Flags< E > flags, E flag ) -> bool
{
    using namespace flag_operators;
    return flags & flag;
}

template < typename E >
constexpr auto no_flags( ) -> Flags< E >
{
    using namespace flag_operators;
    return Flags< E >{ };
}

template < typename E >
constexpr auto all_flags( ) -> Flags< E >
{
    using namespace flag_operators;
    return ~no_flags< E >( );
}

template < template < typename... > typename Container, typename E >
auto to( Flags< E > const& flags )
{

    return magic_enum::enum_values< E >( ) //
         | ranges::views::filter( [ flags ]( E flag ) { return utils::has_flag( flags, flag ); } )
         | ranges::to< Container< E > >( );
}

} // namespace ltb::utils
