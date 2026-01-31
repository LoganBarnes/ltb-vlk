// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_ASSERT_ON_RESULT( ... )                                                         \
    {                                                                                              \
    }

// external
#if defined( __clang__ ) && defined( __apple_build_version__ )
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#include <vulkan/vulkan.hpp>
#pragma clang diagnostic pop
#else
#include <vulkan/vulkan.hpp>
#endif
