// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
// The Linux parts of SharedImage: memory shared as opaque file descriptors.
#if defined( __linux__ )

#include "shared_image_backend.hpp"

// project
#include "ltb/vlk/check.hpp"

namespace ltb::vlk::detail
{

auto external_memory_handle_type( ) -> vk::ExternalMemoryHandleTypeFlagBits
{
    return vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd;
}

auto external_memory_device_extensions( ) -> std::vector< char const* >
{
    return { VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME };
}

auto import_memory( DeviceMemory& memory, DeviceMemorySettings settings, ipc::NativeHandle& handle )
    -> utils::Result< void >
{
    settings.import_fd = handle.get( );
    LTB_CHECK( memory.initialize( settings ) );
    handle.release( ); // Vulkan owns the descriptor once the import succeeds.
    return utils::success( );
}

auto export_memory( Device const& device, DeviceMemory const& memory )
    -> utils::Result< ipc::NativeHandle >
{
    utils::ignore( device );
    LTB_CHECK( auto const fd, memory.export_fd( external_memory_handle_type( ) ) );
    return ipc::NativeHandle{ fd };
}

} // namespace ltb::vlk::detail

#endif // defined( __linux__ )
