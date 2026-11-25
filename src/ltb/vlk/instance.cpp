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

struct SameExtensionName
{
    char const* name;

    auto operator( )( char const* const other ) const -> bool
    {
        return 0 == std::strcmp( name, other );
    }
};

/// \brief True if the extension is not already in the list. Duplicates are not allowed.
struct NotIn
{
    std::vector< char const* > const& extensions;

    auto operator( )( char const* const extension ) const -> bool
    {
        return std::ranges::none_of( extensions, SameExtensionName{ extension } );
    }
};

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
        std::ranges::copy_if(
            glfw->get_vulkan_instance_extensions( ),
            std::back_inserter( extensions ),
            NotIn{ extensions }
        );
    }

    auto create_info = vk::InstanceCreateInfo( )
                           .setFlags( settings.flags )
                           .setPApplicationInfo( &app_info )
                           .setPEnabledExtensionNames( extensions )
                           .setPEnabledLayerNames( settings.layers );

    // The debug messenger structs are only valid if VK_EXT_debug_utils is enabled.
    auto const debug_utils_enabled
        = std::ranges::any_of( extensions, SameExtensionName{ VK_EXT_DEBUG_UTILS_EXTENSION_NAME } );

    if ( debug_utils_enabled )
    {
        // Reports messages from instance creation and destruction.
        create_info.setPNext( &debug_create_info );
    }

    VK_CHECK( auto instance, vk::createInstanceUnique( create_info ) );
    spdlog::debug( "vk::createInstanceUnique()" );

    auto debug_dispatcher = std::unique_ptr< vk::detail::DispatchLoaderDynamic >{ };
    auto debug_messenger  = DebugMessenger{ };

    if ( debug_utils_enabled )
    {
        // Reports messages for the lifetime of the instance. The function is an extension
        // so it has to be loaded at runtime.
        debug_dispatcher = std::make_unique<
            vk::detail::DispatchLoaderDynamic >( instance.get( ), vkGetInstanceProcAddr );
        VK_CHECK(
            debug_messenger,
            instance->createDebugUtilsMessengerEXTUnique(
                debug_create_info,
                nullptr,
                *debug_dispatcher
            )
        );
        spdlog::debug( "vk::createDebugUtilsMessengerEXTUnique()" );
    }

    app_info_         = app_info;
    settings_         = std::move( settings );
    instance_         = std::move( instance );
    debug_dispatcher_ = std::move( debug_dispatcher );
    debug_messenger_  = std::move( debug_messenger );

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
