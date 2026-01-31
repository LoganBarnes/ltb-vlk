// ///////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022 Logan Barnes - All Rights Reserved
// ///////////////////////////////////////////////////////////////////////////////////////
#include "type_string.hpp"

#include <regex>
#ifdef __GNUG__
#include <cstdlib>
#include <cxxabi.h>
#include <memory>

namespace ltb::utils::detail
{

auto demangle( char const* name ) -> std::string
{
    int status = { };
    std::unique_ptr< char, void ( * )( void* ) >
        res{ abi::__cxa_demangle( name, nullptr, nullptr, &status ), std::free };
    return ( status == 0 ) ? res.get( ) : name;
}

#else

namespace ltb::utils::detail
{

// does nothing if not g++
std::string demangle( char const* name )
{
    return name;
}

#endif

namespace
{

constexpr char const* ugly_string1
    = "std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >";

constexpr char const* ugly_string2
    = "std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >";

} // namespace

auto replace_ugly_strings( std::string type_str ) -> std::string
{
    type_str
        = std::regex_replace( type_str, std::regex( ugly_string1 ), std::string( "std::string>" ) );
    type_str
        = std::regex_replace( type_str, std::regex( ugly_string2 ), std::string( "std::string" ) );
    return type_str;
}

} // namespace ltb::utils
