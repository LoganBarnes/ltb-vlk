// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/ipc/native_handle.hpp"
#include "ltb/utils/result.hpp"
#include "ltb/vlk/device_memory.hpp"
#include "ltb/vlk/fwd.hpp"
#include "ltb/vlk/vulkan.hpp"

// standard
#include <vector>

/// The platform-specific parts of SharedImage, for platforms that share memory through
/// Vulkan's external memory handles. Each platform implements these in
/// shared_image_<platform>.cpp:
///
///   Linux    shared_image_fd.cpp     opaque file descriptors (VK_KHR_external_memory_fd)
///   Windows  shared_image_win32.cpp  NT handles (VK_KHR_external_memory_win32)
///
/// (macOS shares IOSurfaces, which don't fit this model, so it would implement SharedImage
/// directly instead.)
#if defined( LTB_IPC_SUPPORTED )

namespace ltb::vlk::detail
{

/// \brief The handle type images are created with and memory is exported as.
auto external_memory_handle_type( ) -> vk::ExternalMemoryHandleTypeFlagBits;

/// \brief Device extensions needed to export and import that handle type.
auto external_memory_device_extensions( ) -> std::vector< char const* >;

/// \brief Allocates `memory` from memory another device or process exported. `settings`
///        has everything but the platform's import info. `handle` may be released (when
///        the import takes ownership of it) or left for the caller to close.
auto import_memory( DeviceMemory& memory, DeviceMemorySettings settings, ipc::NativeHandle& handle )
    -> utils::Result< void >;

/// \brief A new handle to `memory`, which must have been allocated as exportable.
auto export_memory( Device const& device, DeviceMemory const& memory )
    -> utils::Result< ipc::NativeHandle >;

} // namespace ltb::vlk::detail

#endif
