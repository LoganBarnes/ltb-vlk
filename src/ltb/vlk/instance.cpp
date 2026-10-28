// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/vlk/instance.hpp"

// project
#include "ltb/vlk/check.hpp"
#include "ltb/vlk/version.hpp"
#include "ltb/window/glfw_context.hpp"

// external
#include <spdlog/spdlog.h>

// standard
#include <algorithm>
#include <cstring>

namespace ltb::vlk
{
namespace
{

VKAPI_ATTR auto default_debug_callback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT const      message_severity,
    vk::DebugUtilsMessageTypeFlagsEXT const             message_type,
    vk::DebugUtilsMessengerCallbackDataEXT const* const callback_data,
    void* const                                         user_data
) -> vk::Bool32
{
    utils::ignore( message_type, user_data );

    switch ( message_severity )
    {
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
            spdlog::debug( "Validation Layer: {}", callback_data->pMessage );
            break;
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
            spdlog::info( "Validation Layer: {}", callback_data->pMessage );
            break;
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
            spdlog::warn( "Validation Layer: {}", callback_data->pMessage );
            break;
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
            spdlog::error( "Validation Layer: {}", callback_data->pMessage );
            break;
    }

    return false;
}

constexpr auto debug_create_info = vk::DebugUtilsMessengerCreateInfoEXT{ }
                                       .setMessageSeverity(
                                           vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
                                           | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
                                           | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
                                       )
                                       .setMessageType(
                                           vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
                                           | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
                                           | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
                                       )
                                       .setPfnUserCallback( default_debug_callback );

} // namespace

Instance::Instance( )
{
}

auto Instance::initialize( InstanceSettings settings, window::GlfwContext const* const glfw )
    -> utils::Result< void >
{
    if ( this->is_initialized( ) )
    {
        return utils::success( );
    }

    if ( nullptr != glfw )
    {
        LTB_CHECK_VALID( glfw->is_initialized( ) );
    }

    auto const app_info = vk::ApplicationInfo( )
                              .setPApplicationName( settings.app_name.c_str( ) )
                              .setApplicationVersion( VLK_MAKE_API_VERSION( 0, 0, 0, 1 ) )
                              .setPEngineName( "ltb::vlk" )
                              .setEngineVersion( VLK_MAKE_API_VERSION( 0, 0, 0, 1 ) )
                              .setApiVersion( VLK_API_VERSION_1_4 );

    auto extensions = settings.extensions;
    if ( nullptr != glfw )
    {
        auto const glfw_extensions = glfw->get_vulkan_instance_extensions( );

        for ( auto const* const glfw_extension : glfw_extensions )
        {
            // Duplicate extension names are not allowed.
            if ( std::ranges::none_of(
                     extensions,
                     [ glfw_extension ]( char const* const existing )
                     { return 0 == std::strcmp( existing, glfw_extension ); }
                 ) )
            {
                extensions.push_back( glfw_extension );
            }
        }
    }

    auto const create_info = vk::InstanceCreateInfo( )
                                 .setFlags( settings.flags )
                                 .setPApplicationInfo( &app_info )
                                 .setPEnabledExtensionNames( extensions )
                                 .setPEnabledLayerNames( settings.layers )
                                 .setPNext( debug_create_info );

    VK_CHECK( auto instance, vk::createInstanceUnique( create_info ) );
    spdlog::debug( "vk::createInstanceUnique()" );

    app_info_ = app_info;
    settings_ = std::move( settings );
    instance_ = std::move( instance );

    // Point at the stored string since the original may have been moved (short strings).
    app_info_.setPApplicationName( settings_.app_name.c_str( ) );

    return utils::success( );
}

auto Instance::is_initialized( ) const -> bool
{
    return nullptr != instance_.get( );
}

auto Instance::get( ) const -> vk::Instance const&
{
    return instance_.get( );
}

auto Instance::get( ) -> vk::Instance&
{
    return instance_.get( );
}

auto Instance::settings( ) const -> InstanceSettings const&
{
    return settings_;
}

auto Instance::app_info( ) const -> vk::ApplicationInfo const&
{
    return app_info_;
}

} // namespace ltb::vlk
