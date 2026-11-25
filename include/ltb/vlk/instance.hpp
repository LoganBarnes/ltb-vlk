// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/utils/result.hpp"
#include "ltb/vlk/vulkan.hpp"
#include "ltb/window/fwd.hpp"

// standard
#include <memory>

namespace ltb::vlk
{

struct InstanceSettings
{
    std::string app_name = "Vulkan Application";

    vk::InstanceCreateFlags flags = {
#if defined( __APPLE__ )
        vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR
#endif
    };

    std::vector< char const* > extensions = {
#if defined( __APPLE__ )
        VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
#endif
#if !defined( NDEBUG )
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
    };

    std::vector< char const* > layers = {
#if !defined( NDEBUG ) && !defined( WIN32 )
        "VK_LAYER_KHRONOS_validation",
#endif
    };
};

class Instance
{
public:
    Instance( );

    auto initialize( InstanceSettings settings, window::GlfwContext const* glfw )
        -> utils::Result< void >;

    [[nodiscard( "Const getter" )]]
    auto is_initialized( ) const -> bool;

    [[nodiscard( "Const getter" )]]
    auto get( ) const -> vk::Instance const&;
    auto get( ) -> vk::Instance&;

    [[nodiscard( "Const getter" )]]
    auto settings( ) const -> InstanceSettings const&;

    [[nodiscard( "Const getter" )]]
    auto app_info( ) const -> vk::ApplicationInfo const&;

private:
    using DebugMessenger
        = vk::UniqueHandle< vk::DebugUtilsMessengerEXT, vk::detail::DispatchLoaderDynamic >;

    vk::ApplicationInfo app_info_ = { };
    InstanceSettings    settings_ = { };
    vk::UniqueInstance  instance_ = { };

    // Only set if VK_EXT_debug_utils is enabled. Destroyed before the instance (member order).
    // The dispatcher is heap allocated because the messenger keeps a pointer to it.
    std::unique_ptr< vk::detail::DispatchLoaderDynamic > debug_dispatcher_ = nullptr;
    DebugMessenger                                       debug_messenger_  = { };
};

} // namespace ltb::vlk
