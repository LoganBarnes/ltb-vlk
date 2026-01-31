// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/utils/macro.hpp"
#include "ltb/vlk/vulkan.hpp"

/// \brief Returns a tl::unexpected if `func` returns an error Result
#define VK_CHECK1( unique_name, func )                                                             \
    do                                                                                             \
    {                                                                                              \
        if ( auto unique_name = ( func ); unique_name != vk::Result::eSuccess )                    \
        {                                                                                          \
            return LTB_MAKE_UNEXPECTED_ERROR(                                                      \
                "{} failed: {}",                                                                   \
                #func,                                                                             \
                vk::to_string( unique_name )                                                       \
            );                                                                                     \
        }                                                                                          \
    } while ( false )

/// \brief Returns a tl::unexpected if `func` returns an
///        error, otherwise assigns the result to `var`.
#define VK_CHECK2( unique_name, var, func )                                                        \
    auto&& unique_name = ( func );                                                                 \
    if ( unique_name.result != vk::Result::eSuccess )                                              \
    {                                                                                              \
        return LTB_MAKE_UNEXPECTED_ERROR(                                                          \
            "{} failed: {}",                                                                       \
            #func,                                                                                 \
            vk::to_string( unique_name.result )                                                    \
        );                                                                                         \
    }                                                                                              \
    var = std::move( unique_name.value )

/// \brief An "overloaded" macro that is replaced by VK_CHECK1
///        or VK_CHECK2 depending on the number of arguments.
#define VK_CHECK( ... )                                                                            \
    LTB_EXPAND( LTB_GET2( __VA_ARGS__, VK_CHECK2, VK_CHECK1, )(                                    \
        LTB_UNIQUE_NAME( vk_check_macro_result ),                                                  \
        __VA_ARGS__                                                                                \
    ) )
