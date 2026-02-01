// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/utils/error.hpp"
#include "ltb/utils/ignore.hpp"
#include "ltb/utils/macro.hpp"

namespace tl
{

template < class T, class E >
class [[nodiscard]] expected;

}

// external
#include <tl/expected.hpp>

// Macros to reduce typing when errors occur:

#define LTB_MAKE_UNEXPECTED_ERROR( ... ) ::tl::make_unexpected( LTB_MAKE_ERROR( __VA_ARGS__ ) )
#define LTB_MAKE_UNEXPECTED_WARNING( ... ) ::tl::make_unexpected( LTB_MAKE_WARNING( __VA_ARGS__ ) )

// Do not use this macro directly; use LTB_CHECK_VALID instead.
#define DETAIL_LTB_CHECK_VALID1( var )                                                             \
    do                                                                                             \
    {                                                                                              \
        if ( !( var ) )                                                                            \
        {                                                                                          \
            return LTB_MAKE_UNEXPECTED_ERROR( "{} invalid", #var );                                \
        }                                                                                          \
    } while ( false )

// Do not use this macro directly; use LTB_CHECK_VALID instead.
#define DETAIL_LTB_CHECK_VALID2( var, msg )                                                        \
    do                                                                                             \
    {                                                                                              \
        if ( !( var ) )                                                                            \
        {                                                                                          \
            return LTB_MAKE_UNEXPECTED_ERROR( "{} invalid: {}", #var, msg );                       \
        }                                                                                          \
    } while ( false )

/// \brief Checks if the inputs expression is valid,
///        otherwise returns an unexpected error.
#define LTB_CHECK_VALID( ... )                                                                     \
    LTB_EXPAND(                                                                                    \
        LTB_GET2( __VA_ARGS__, DETAIL_LTB_CHECK_VALID2, DETAIL_LTB_CHECK_VALID1, )( __VA_ARGS__ )  \
    )

// Do not use this macro directly; use LTB_CHECK_VALID_OR instead.
#define DETAIL_LTB_CHECK_VALID_OR( var, callback )                                                 \
    do                                                                                             \
    {                                                                                              \
        if ( !( var ) )                                                                            \
        {                                                                                          \
            callback( LTB_MAKE_ERROR( "{} invalid", #var ) );                                      \
        }                                                                                          \
    } while ( false )

// Do not use this macro directly; use LTB_CHECK_VALID_OR instead.
#define DETAIL_LTB_CHECK_VALID_OR( var, callback )                                                 \
    do                                                                                             \
    {                                                                                              \
        if ( !( var ) )                                                                            \
        {                                                                                          \
            callback( LTB_MAKE_ERROR( "{} invalid", #var ) );                                      \
        }                                                                                          \
    } while ( false )

/// \brief Checks if the inputs expression is valid,
///        otherwise calls the provided error callback.
#define LTB_CHECK_VALID_OR( ... )                                                                  \
    LTB_EXPAND( LTB_GET2( __VA_ARGS__, DETAIL_LTB_CHECK_VALID_OR2, DETAIL_LTB_CHECK_VALID_OR1, )(  \
        __VA_ARGS__                                                                                \
    ) )

// Do not use this macro directly; use LTB_CHECK instead.
#define DETAIL_LTB_CHECK1( unique_name, func )                                                     \
    do                                                                                             \
    {                                                                                              \
        if ( auto unique_name = to_void( func ); /* check for error -> */ !unique_name )           \
        {                                                                                          \
            return tl::make_unexpected( unique_name.error( ) );                                    \
        }                                                                                          \
    } while ( false )

// Do not use this macro directly; use LTB_CHECK instead.
#define DETAIL_LTB_CHECK2( unique_name, var, func )                                                \
    auto&& unique_name = ( func );                                                                 \
    if ( !unique_name )                                                                            \
    {                                                                                              \
        return tl::make_unexpected( unique_name.error( ) );                                        \
    }                                                                                              \
    var = std::move( unique_name.value( ) )

/// \brief An "overloaded" macro that is replaced by LTB_CHECK1
///        or LTB_CHECK2 depending on the number of arguments.
#define LTB_CHECK( ... )                                                                           \
    LTB_EXPAND( LTB_GET2( __VA_ARGS__, DETAIL_LTB_CHECK2, DETAIL_LTB_CHECK1, )(                    \
        LTB_UNIQUE_NAME( ltb_check_macro_result ),                                                 \
        __VA_ARGS__                                                                                \
    ) )

// Do not use this macro directly; use LTB_CHECK_OF instead.
#define DETAIL_LTB_CHECK_OR1( unique_name, func, callback )                                        \
    do                                                                                             \
    {                                                                                              \
        if ( auto unique_name = to_void( func ); /* check for error -> */ !unique_name )           \
        {                                                                                          \
            callback( unique_name.error( ) );                                                      \
        }                                                                                          \
    } while ( false )

// Do not use this macro directly; use LTB_CHECK_OF instead.
#define DETAIL_LTB_CHECK_OR2( unique_name, var, func, callback )                                   \
    auto&& unique_name = ( func );                                                                 \
    if ( !unique_name )                                                                            \
    {                                                                                              \
        callback( unique_name.error( ) );                                                          \
    }                                                                                              \
    var = std::move( unique_name.value( ) )

/// \brief An "overloaded" macro that is replaced by LTB_CHECK_OR1
///        or LTB_CHECK_OR2 depending on the number of arguments.
#define LTB_CHECK_OR( ... )                                                                        \
    LTB_EXPAND( LTB_GET3( __VA_ARGS__, DETAIL_LTB_CHECK_OR2, DETAIL_LTB_CHECK_OR1, )(              \
        LTB_UNIQUE_NAME( ltb_check_macro_result ),                                                 \
        __VA_ARGS__                                                                                \
    ) )

namespace ltb::utils
{

/// \brief A type of tl::expected that requires the result to be used.
///
/// Example usage:
///
/// \code{.cpp}
///
/// // function that returns an int or 'ltb::util::Error'
/// auto my_function(int non_negative) -> util::Result<int> {
///     if (non_negative < 0) {
///         return tl::make_unexpected(LTB_MAKE_ERROR("number is negative"));
///     }
///     return non_negative; // Success
/// }
///
/// int main() {
///     // compilation error since the return value is not used (C++17)
///     my_function(0);
///
///     // compilation error since 'result1' is unused
///     auto result1 = my_function(1);
///
///     // OK, result is used
///     auto result2 = my_function(2);
///     if (result2) {
///     }
///
///     // OK, throws on error
///     my_function(3).value();
///
///     // OK, clearer error handling
///     my_function(4).or_else(util::throw_error<>).value();
///
///     // OK
///     my_function(5).map([](auto i) { return std::to_string(i); }).value_or(std::string("NEG"));
///
///     // OK, but not recommended
///     util::ignore(my_function(6));
///
///     // OK, but not recommended
///     auto result3 = my_function(7);
///     ltb::util::ignore(result3);
///
///     return 0;
/// }
///
/// \endcode
template < typename T = void, typename E = Error >
using Result = tl::expected< T, E >;

auto success( ) -> Result< void >;

/// \brief A wrapper around `util::Result`s to show the result has
///        been handled and to suppress the related compiler warning.
template < typename... Args >
void handled( Args&&... )
{
}

/// Convert a result to a void result
template < typename E >
auto to_void( Result< void, E > const& result ) -> Result< void >
{
    return result;
}

/// Convert a result to a void result
template < typename T, typename E >
auto to_void( Result< T, E > const& result ) -> Result< void >
{
    return result.map( []( auto const& ) {} );
}

} // namespace ltb::utils
