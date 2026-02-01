// ///////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022 Logan Barnes - All Rights Reserved
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// standard
#include <variant>

namespace ltb::utils
{

/**
 * Wrapper around std::visit that forces the second parameter to be a std::variant class. This works
 * with objects implicitly convertible into variants (like classes deriving from std::variant).
 */
template < typename Visitor, typename... Ts >
constexpr auto visit( Visitor&& visitor, std::variant< Ts... >& variants )
{
    return std::visit( std::forward< Visitor >( visitor ), variants );
}

template < typename Visitor, typename... Ts >
constexpr auto visit( Visitor&& visitor, std::variant< Ts... > const& variants )
{
    return std::visit( std::forward< Visitor >( visitor ), variants );
}

template < class... Ts >
struct Visitor : Ts...
{
    using Ts::operator( )...;
};

template < class... Ts >
Visitor( Ts... ) -> Visitor< Ts... >;

} // namespace ltb::utils
