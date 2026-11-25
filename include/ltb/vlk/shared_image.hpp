// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/ipc/native_handle.hpp"
#include "ltb/utils/result.hpp"
#include "ltb/utils/types.hpp"
#include "ltb/vlk/device_memory.hpp"
#include "ltb/vlk/fwd.hpp"
#include "ltb/vlk/image.hpp"
#include "ltb/vlk/vulkan.hpp"

// standard
#include <type_traits>
#include <vector>

/// Images whose memory is shared between processes (or between devices in one process).
///
/// One side calls `create()` and `export_handle()`, sends the handle (with a Channel) and
/// `info()` to the other side, which calls `import()`. Both sides then have a `vk::Image`
/// backed by the same memory. Only images on the same GPU and driver can share memory
/// (compare `PhysicalDevice::identity()`).
///
/// Backends:
///
///   Linux    Opaque file descriptors (VK_KHR_external_memory_fd).
///   Windows  NT handles (VK_KHR_external_memory_win32, eOpaqueWin32), exported with
///            vkGetMemoryWin32HandleKHR. Not yet built or tested on Windows.
///   macOS    Not implemented yet. Plan: IOSurfaces (VK_EXT_metal_objects). `create()` makes
///            an IOSurface and imports it into Vulkan, `export_handle()` returns its Mach port
///            (IOSurfaceCreateMachPort), and `import()` looks the surface up from the port.
///
/// Linux and Windows share shared_image.cpp and only differ in the few functions in
/// src/ltb/vlk/shared_image_backend.hpp (shared_image_fd.cpp, shared_image_win32.cpp).
/// macOS doesn't fit that model, so it would implement this header directly instead.
/// Either way, callers don't change.
namespace ltb::vlk
{

struct SharedImageSettings
{
    vk::Extent2D        extent = { };
    vk::Format          format = vk::Format::eUndefined;
    vk::ImageUsageFlags usage  = { };
};

/// \brief Everything besides the handle that another process needs to import the image.
///        Trivially copyable, so it can be sent as part of a message.
struct SharedImageInfo
{
    SharedImageSettings settings          = { };
    uint32              memory_type_index = 0U;
    uint64              allocation_size   = 0U;
};

static_assert( std::is_trivially_copyable_v< SharedImageInfo > );

/// \brief The device extensions shared images need on this platform. Add them to
///        `DeviceSettings::extensions` on both sides.
auto shared_image_device_extensions( ) -> std::vector< char const* >;

#if defined( LTB_IPC_SUPPORTED )

class SharedImage
{
public:
    explicit( false ) SharedImage( Device& device );

    /// \brief Creates an image whose memory can be exported with `export_handle()`.
    auto create( SharedImageSettings settings ) -> utils::Result< void >;

    /// \brief Creates an image backed by memory another process (or device) exported.
    ///        Takes ownership of `handle`, whether or not the import succeeds.
    auto import( SharedImageInfo const& info, ipc::NativeHandle handle ) -> utils::Result< void >;

    /// \brief A new handle to the image's memory for another process. The caller owns it,
    ///        and can send it with a Channel (which gives the receiver its own copy).
    [[nodiscard( "Const computation" )]]
    auto export_handle( ) const -> utils::Result< ipc::NativeHandle >;

    [[nodiscard( "Const getter" )]]
    auto is_initialized( ) const -> bool;

    [[nodiscard( "Const getter" )]]
    auto info( ) const -> SharedImageInfo const&;

    [[nodiscard( "Const getter" )]]
    auto image( ) const -> Image const&;
    auto image( ) -> Image&;

    auto reset( ) -> void;

private:
    Device& device_;

    Image           image_       = { device_ };
    DeviceMemory    memory_      = { device_ };
    SharedImageInfo info_        = { };
    bool            initialized_ = false;
};

#endif

} // namespace ltb::vlk
