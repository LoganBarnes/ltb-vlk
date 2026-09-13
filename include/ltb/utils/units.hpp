// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <type_traits>

namespace ltb
{

// TODO: Use std::ratio and std::chrono somehow?

// These acronyms are referenced in docs/StyleGuide.md (Unit Conversions)
// so please update that file if this list is moved.
//
// ACRONYM REFERENCE
//  hr:   hours
//  min:  minutes
//  s:    seconds
//  mi:   miles
//  nm:   nautical miles
//  m:    meters
//  cm:   centimeters
//  in:   inches
//  ft:   feet
//  pa:   Pascals
//  mb:   millibars
//  inhg: inches of mercury
//  2:    squared
//  3:    cubed
//  kts:  nautical miles per hour (knots)
// EXAMPLES
//  cm3:        centimeters cubed
//  m_per_s:    meters per second
//  mi_per_hr:  miles per hour
//  ft_per_min: feet per minute
//  m_per_s2:   meters per second squared
//
// NOTE:
//  Using the `m_per_s` syntax instead of the `mps` syntax avoids confusion
//  with abbreviations like mph (which is commonly miles per hour). Instead,
//  `mi_per_hr` is miles per hour and `m_per_hr` is meters per hour.

namespace constants
{

// Time constants (integral, exact — for integer tick arithmetic)
namespace integer
{

constexpr auto sec_from_min     = 60;
constexpr auto sec_from_hour    = 60 * sec_from_min;
constexpr auto sec_from_day     = 24 * sec_from_hour;
constexpr auto sec_from_week    = 7 * sec_from_day;
constexpr auto nano_from_micro  = 1'000;
constexpr auto micro_from_milli = 1'000;
constexpr auto nano_from_milli  = 1'000'000;
constexpr auto nano_from_sec    = 1'000'000'000;

} // namespace integer

// Time constants
constexpr auto hours_from_day = 24.0;
constexpr auto min_from_hr    = 60.0;
constexpr auto s_from_min     = 60.0;
constexpr auto ms_from_s      = 1'000.0;
constexpr auto s_from_hr      = s_from_min * min_from_hr;
constexpr auto us_from_ms     = 1'000.0;
constexpr auto us_from_s      = us_from_ms * ms_from_s;

constexpr auto hr_from_min = 1.0 / min_from_hr;
constexpr auto min_from_s  = 1.0 / s_from_min;
constexpr auto s_from_ms   = 1.0 / ms_from_s;
constexpr auto hr_from_s   = 1.0 / s_from_hr;
constexpr auto ms_from_us  = 1.0 / us_from_ms;

// Distance constants
constexpr auto in_from_ft = 12.0;
constexpr auto ft_from_mi = 5'280.0;
constexpr auto m_from_nm  = 1'852.0;
constexpr auto cm_from_in = 2.54;
constexpr auto cm_from_m  = 100.0;
constexpr auto mm_from_m  = 1000.0;

constexpr auto m_from_cm = 1.0 / cm_from_m;
constexpr auto m_from_mm = 1.0 / mm_from_m;
constexpr auto nm_from_m = 1.0 / m_from_nm;

constexpr auto m_from_ft  = m_from_cm * cm_from_in * in_from_ft;
constexpr auto nm_from_mi = nm_from_m * m_from_ft * ft_from_mi;

constexpr auto ft_from_m  = 1.0 / m_from_ft;
constexpr auto mi_from_nm = 1.0 / nm_from_mi;
constexpr auto mi_from_ft = 1.0 / ft_from_mi;

// Speed constants
constexpr auto m_per_s_from_kts        = m_from_nm / s_from_hr;
constexpr auto ft_per_min_from_m_per_s = ft_from_m / min_from_s;

constexpr auto kts_from_m_per_s        = 1.0 / m_per_s_from_kts;
constexpr auto m_per_s_from_ft_per_min = 1.0 / ft_per_min_from_m_per_s;

// Pressure constants
// https://www.weather.gov/media/epz/wxcalc/pressureConversion.pdf
constexpr auto inhg_from_kpa                  = 0.2953;
constexpr auto inhg_from_hpa                  = 0.02953;
constexpr auto inhg_from_mb                   = 0.02953;
constexpr auto inhg_from_pa                   = 0.0002953;
constexpr auto kpa_from_inhg                  = 1.0 / inhg_from_kpa;
constexpr auto mb_from_inhg                   = 1.0 / inhg_from_mb;
constexpr auto pa_from_inhg                   = 1.0 / inhg_from_pa;
constexpr auto kpa_from_psi                   = 6.89476;
constexpr auto psi_from_kpa                   = 1.0 / kpa_from_psi;
constexpr auto mb_from_pa                     = 0.01;
constexpr auto kpa_from_mb                    = 0.1;
constexpr auto mb_from_kpa                    = 1.0 / kpa_from_mb;
constexpr auto psi_from_mb                    = kpa_from_mb * psi_from_kpa;
constexpr auto mb_from_psi                    = 1.0 / psi_from_mb;
constexpr auto default_altimeter_setting_inhg = 29.9213;

// Fuel specific gravity
constexpr auto fuel_litres_from_kg = 1.0 / 0.72;
constexpr auto gals_from_litres    = 0.2641720524;

// Network constants
constexpr auto bps_from_kbps   = 1'000.0;
constexpr auto kbps_from_bps   = 1.0 / bps_from_kbps;
constexpr auto bits_from_bytes = 8.0;
constexpr auto bytes_from_bits = 1.0 / bits_from_bytes;

// Generic math constants
constexpr auto decimal_from_percentage       = 0.01;
constexpr auto percentage_from_decimal       = 1.0 / decimal_from_percentage;
constexpr auto unit_from_milli_unit          = 0.001;
constexpr auto percentage_from_milli_decimal = percentage_from_decimal * unit_from_milli_unit;

// Temperature
constexpr auto celsius_at_zero_kelvin = -273.15;

} // namespace constants

namespace utils
{

template < typename T, std::enable_if_t< std::is_floating_point_v< T >, int > = 0 >
T constexpr kts_from_m_per_s( T m_per_s )
{
    return static_cast< T >( constants::kts_from_m_per_s ) * m_per_s;
}

template < typename T, std::enable_if_t< std::is_floating_point_v< T >, int > = 0 >
T constexpr m_per_s_from_kts( T kts )
{
    return static_cast< T >( constants::m_per_s_from_kts ) * kts;
}

template < typename T, std::enable_if_t< std::is_floating_point_v< T >, int > = 0 >
T constexpr ft_per_min_from_m_per_s( T m_per_s )
{
    return static_cast< T >( constants::ft_per_min_from_m_per_s ) * m_per_s;
}

template < typename T, std::enable_if_t< std::is_floating_point_v< T >, int > = 0 >
T constexpr m_per_s_from_ft_per_min( T ft_per_min )
{
    return static_cast< T >( constants::m_per_s_from_ft_per_min ) * ft_per_min;
}

template < typename T, std::enable_if_t< std::is_floating_point_v< T >, int > = 0 >
T constexpr ms_from_us( T us )
{
    return static_cast< T >( constants::ms_from_us ) * us;
}

template < typename T, std::enable_if_t< std::is_floating_point_v< T >, int > = 0 >
T constexpr us_from_ms( T ms )
{
    return static_cast< T >( constants::us_from_ms ) * ms;
}

template < typename T >
T constexpr decimal_from_percentage( T percentage )
{
    return static_cast< T >( constants::decimal_from_percentage ) * percentage;
}

template < typename T >
T constexpr percentage_from_decimal( T decimal )
{
    return static_cast< T >(
        constants::percentage_from_decimal * static_cast< double >( decimal )
    );
}

template < typename T >
T constexpr bps_from_kbps( T kbps )
{
    return static_cast< T >( constants::bps_from_kbps ) * kbps;
}

template < typename T >
T constexpr kbps_from_bps( T bps )
{
    return static_cast< T >( constants::kbps_from_bps * static_cast< double >( bps ) );
}

template < typename T >
T constexpr bits_from_bytes( T bytes )
{
    return static_cast< T >( constants::bits_from_bytes ) * bytes;
}

template < typename T >
T constexpr bytes_from_bits( T bits )
{
    return static_cast< T >( constants::bytes_from_bits * static_cast< double >( bits ) );
}

/// \brief Converts fahrenheit to celsius.
template < typename In, typename Out = In >
auto constexpr c_from_f( In const fahrenheit )
{
    return ( static_cast< Out >( fahrenheit ) - static_cast< Out >( 32.0 ) )
         * static_cast< Out >( 5.0 / 9.0 );
}

/// \brief Converts celsius to fahrenheit.
template < typename In, typename Out = In >
auto constexpr f_from_c( In const celsius )
{
    return ( static_cast< Out >( celsius ) * static_cast< Out >( 9.0 / 5.0 ) )
         + static_cast< Out >( 32.0 );
}

/// \brief Converts kelvin to celsius
template < typename In, typename Out = In >
auto constexpr c_from_k( In const kelvin )
{
    return static_cast< Out >( kelvin ) - static_cast< Out >( 273.15 );
}

/// \brief Converts celsius to kelvin
template < typename In, typename Out = In >
auto constexpr k_from_c( In const celsius )
{
    return static_cast< Out >( celsius ) + static_cast< Out >( 273.15 );
}

/// \brief Converts millidegrees Celsius to degrees Celsius.
template < typename In, typename Out = In >
auto constexpr c_from_milli_c( In const milli_celsius )
{
    return static_cast< Out >( milli_celsius ) / static_cast< Out >( 1000.0 );
}

} // namespace utils
} // namespace ltb
