// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/utils/result.hpp"
#include "ltb/vlk/vulkan.hpp"
#include "ltb/window/fwd.hpp"

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
    vk::ApplicationInfo app_info_ = { };
    InstanceSettings    settings_ = { };
    vk::UniqueInstance  instance_ = { };
};

} // namespace ltb::vlk
