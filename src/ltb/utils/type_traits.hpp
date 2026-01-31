// ///////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022 Logan Barnes - All Rights Reserved
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <type_traits>

namespace ltb::utils
{
namespace detail
{

template < typename T >
auto is_dereferencable( int ) -> decltype( *std::declval< T >( ), std::true_type{ } );

template < typename >
auto is_dereferencable( unsigned long ) -> std::false_type;

template < typename T >
auto is_boolable( int ) -> decltype( !std::declval< T >( ), std::true_type{ } );

template < typename >
auto is_boolable( unsigned long ) -> std::false_type;

} // namespace detail

template < typename T >
using is_dereferencable = decltype( detail::is_dereferencable< T >( 0 ) );

template < typename T >
constexpr auto is_dereferencable_v = is_dereferencable< T >::value;

template < typename T >
using is_boolable = decltype( detail::is_boolable< T >( 0 ) );

template < typename T >
constexpr auto is_boolable_v = is_boolable< T >::value;

template < typename T >
using dereferenced_t = std::decay_t< decltype( *std::declval< T >( ) ) >;

template < typename T >
using is_optional = std::conjunction< is_dereferencable< T >, is_boolable< T > >;

template < typename T >
constexpr auto is_optional_v = is_optional< T >::value;

} // namespace ltb::utils
