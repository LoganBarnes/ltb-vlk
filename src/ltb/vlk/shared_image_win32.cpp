// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
// The Windows parts of SharedImage: memory shared as NT handles
// (VK_KHR_external_memory_win32).
#if defined( _WIN32 )

// The Win32 parts of the Vulkan headers are only declared when this is defined before
// they're first included, which happens through the project headers below.
#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "shared_image_backend.hpp"

// project
#include "ltb/vlk/check.hpp"
#include "ltb/vlk/device.hpp"

namespace ltb::vlk::detail
{

auto external_memory_handle_type( ) -> vk::ExternalMemoryHandleTypeFlagBits
{
    // NT handles (as opposed to the older, global eOpaqueWin32Kmt handles). They're
    // reference counted and only usable in processes they're duplicated into.
    return vk::ExternalMemoryHandleTypeFlagBits::eOpaqueWin32;
}

auto external_memory_device_extensions( ) -> std::vector< char const* >
{
    return { VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME };
}

auto import_memory( DeviceMemory& memory, DeviceMemorySettings settings, ipc::NativeHandle& handle )
    -> utils::Result< void >
{
    auto const import_info = vk::ImportMemoryWin32HandleInfoKHR{ }
                                 .setHandleType( external_memory_handle_type( ) )
                                 .setHandle( static_cast< HANDLE >( handle.get( ) ) );
    settings.p_next        = &import_info;

    // Unlike file descriptors, importing an NT handle doesn't give it to Vulkan: the caller
    // still owns it and closes it (WinHandle does when it goes out of scope).
    return memory.initialize( settings );
}

auto export_memory( Device const& device, DeviceMemory const& memory )
    -> utils::Result< ipc::NativeHandle >
{
    LTB_CHECK_VALID( memory.is_initialized( ) );
    LTB_CHECK_VALID(
        static_cast< bool >(
            memory.settings( ).export_handle_types & external_memory_handle_type( )
        ),
        "memory wasn't allocated as exportable"
    );

    // An extension function, so it isn't exported by the Vulkan loader and must be looked up.
    auto const get_memory_win32_handle = reinterpret_cast< PFN_vkGetMemoryWin32HandleKHR >(
        device.get( ).getProcAddr( "vkGetMemoryWin32HandleKHR" )
    );
    LTB_CHECK_VALID(
        get_memory_win32_handle,
        "VK_KHR_external_memory_win32 isn't enabled on the device"
    );

    auto const handle_info = vk::MemoryGetWin32HandleInfoKHR{ }
                                 .setMemory( memory.get( ) )
                                 .setHandleType( external_memory_handle_type( ) );

    auto       handle = HANDLE{ nullptr };
    auto const result = vk::Result{ get_memory_win32_handle(
        device.get( ),
        reinterpret_cast< VkMemoryGetWin32HandleInfoKHR const* >( &handle_info ),
        &handle
    ) };
    if ( vk::Result::eSuccess != result )
    {
        return LTB_MAKE_UNEXPECTED_ERROR(
            "vkGetMemoryWin32HandleKHR failed: {}",
            vk::to_string( result )
        );
    }
    return ipc::NativeHandle{ handle };
}

} // namespace ltb::vlk::detail

#endif // defined( _WIN32 )
