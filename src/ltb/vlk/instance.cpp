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
        auto const previous_size   = extensions.size( );

        auto const inserted_iter = extensions.insert(
            extensions.end( ),
            // append glfw extensions
            glfw_extensions.begin( ),
            glfw_extensions.end( )
        );

        if ( std::cmp_not_equal( previous_size, inserted_iter - extensions.begin( ) ) )
        {
            return LTB_MAKE_UNEXPECTED_ERROR( "Failed to insert glfw_extensions" );
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
