// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
// SharedImage on macOS: images backed by IOSurfaces, shared as Mach ports.
//
// MoltenVK can't export Vulkan memory to other processes, but it can back an image with
// an IOSurface (VK_EXT_metal_objects), and IOSurfaces can be shared between processes:
//
//   create()         Creates an image with VkImportMetalIOSurfaceInfoEXT and no surface,
//                    which tells MoltenVK to make a new IOSurface for it.
//   export_handle()  Gets that surface back (vkExportMetalObjectsEXT) and returns a Mach
//                    port send right for it (IOSurfaceCreateMachPort).
//   import()         Looks the surface up from the port (IOSurfaceLookupFromMachPort) and
//                    creates an image backed by it.
//
// Each side still allocates (and binds) its own VkDeviceMemory, as Vulkan requires, but
// the image's contents live in the IOSurface. So `SharedImageInfo::memory_type_index`
// and `allocation_size` are only informational here.
//
// This doesn't fit shared_image_backend.hpp (nothing is exported through Vulkan's external
// memory API), so it implements shared_image.hpp directly.
#if defined( __APPLE__ )

// The Metal parts of the Vulkan headers are only declared when this is defined before
// they're first included, which happens through the project headers below.
#ifndef VK_USE_PLATFORM_METAL_EXT
#define VK_USE_PLATFORM_METAL_EXT
#endif

#include "ltb/vlk/shared_image.hpp"

// project
#include "ltb/vlk/check.hpp"
#include "ltb/vlk/device.hpp"
#include "ltb/vlk/physical_device.hpp"

// system
#include <CoreFoundation/CoreFoundation.h>
#include <IOSurface/IOSurfaceRef.h>

// standard
#include <memory>
#include <type_traits>

namespace ltb::vlk
{
namespace
{

struct CFReleaser
{
    auto operator( )( void const* const object ) const -> void
    {
        ::CFRelease( object );
    }
};

/// \brief An IOSurface reference that's released when destroyed.
using IOSurfacePtr = std::unique_ptr< std::remove_pointer_t< IOSurfaceRef >, CFReleaser >;

auto check_settings( SharedImageSettings const& settings ) -> utils::Result< void >
{
    LTB_CHECK_VALID( ( settings.extent.width > 0U ) && ( settings.extent.height > 0U ) );
    LTB_CHECK_VALID( vk::Format::eUndefined != settings.format );
    LTB_CHECK_VALID( static_cast< bool >( settings.usage ) );
    return utils::success( );
}

/// \brief Creates `image` backed by `surface`, or by a new IOSurface if it's null. Either
///        way the image keeps its own reference to the surface.
auto create_image( Image& image, SharedImageSettings const& settings, IOSurfaceRef const surface )
    -> utils::Result< void >
{
    auto const import_info = vk::ImportMetalIOSurfaceInfoEXT{ }.setIoSurface( surface );

    // Lets export_handle() get the surface back, for imported images too.
    auto const export_info
        = vk::ExportMetalObjectCreateInfoEXT{ }
              .setPNext( &import_info )
              .setExportObjectType( vk::ExportMetalObjectTypeFlagBitsEXT::eMetalIosurface );

    return image.initialize( {
        .extent = { settings.extent.width, settings.extent.height, 1U },
        .format = settings.format,
        .usage  = settings.usage,
        .p_next = &export_info,
    } );
}

/// \brief Allocates and binds memory for the image, as Vulkan requires. Its contents are in
///        the IOSurface, not this memory.
auto bind_memory( Device& device, Image const& image, DeviceMemory& memory )
    -> utils::Result< SharedImageInfo >
{
    auto const requirements = image.memory_requirements( );
    LTB_CHECK(
        auto const memory_type_index,
        device.physical_device( ).find_memory_type_index(
            { requirements },
            vk::MemoryPropertyFlagBits::eDeviceLocal
        )
    );
    LTB_CHECK( memory.initialize( {
        .allocation_size   = requirements.size,
        .memory_type_index = memory_type_index,
        .dedicated_image   = image.get( ),
    } ) );
    VK_CHECK( device.get( ).bindImageMemory( image.get( ), memory.get( ), 0U ) );

    return SharedImageInfo{
        .memory_type_index = memory_type_index,
        .allocation_size   = requirements.size,
    };
}

/// \brief The IOSurface backing an image created by `create_image`. Not retained.
auto backing_surface( Device const& device, Image const& image ) -> utils::Result< IOSurfaceRef >
{
    // An extension function, so it isn't exported by the Vulkan loader and must be looked up.
    auto const export_metal_objects = reinterpret_cast< PFN_vkExportMetalObjectsEXT >(
        device.get( ).getProcAddr( "vkExportMetalObjectsEXT" )
    );
    LTB_CHECK_VALID( export_metal_objects, "VK_EXT_metal_objects isn't enabled on the device" );

    auto surface_info = vk::ExportMetalIOSurfaceInfoEXT{ }.setImage( image.get( ) );
    auto objects_info = vk::ExportMetalObjectsInfoEXT{ }.setPNext( &surface_info );
    export_metal_objects(
        device.get( ),
        reinterpret_cast< VkExportMetalObjectsInfoEXT* >( &objects_info )
    );

    LTB_CHECK_VALID( nullptr != surface_info.ioSurface, "the image isn't backed by an IOSurface" );
    return surface_info.ioSurface;
}

} // namespace

auto shared_image_device_extensions( ) -> std::vector< char const* >
{
    return { VK_EXT_METAL_OBJECTS_EXTENSION_NAME };
}

SharedImage::SharedImage( Device& device )
    : device_( device )
{
}

auto SharedImage::create( SharedImageSettings settings ) -> utils::Result< void >
{
    if ( this->is_initialized( ) )
    {
        return utils::success( );
    }
    LTB_CHECK_VALID( device_.is_initialized( ) );
    LTB_CHECK( check_settings( settings ) );

    // Undo any partial initialization on failure so create() can be retried.
    auto const create_resources = [ this, &settings ]( ) -> utils::Result< void > {
        LTB_CHECK( create_image( image_, settings, nullptr ) );
        LTB_CHECK( auto info, bind_memory( device_, image_, memory_ ) );

        // Fail now, rather than in export_handle(), if MoltenVK didn't make a surface.
        LTB_CHECK( backing_surface( device_, image_ ) );

        info.settings = settings;
        info_         = info;
        return utils::success( );
    };

    if ( auto result = create_resources( ); !result )
    {
        this->reset( );
        return result;
    }
    initialized_ = true;
    return utils::success( );
}

auto SharedImage::import( SharedImageInfo const& info, ipc::NativeHandle handle )
    -> utils::Result< void >
{
    LTB_CHECK_VALID( !this->is_initialized( ), "already holds an image" );
    LTB_CHECK_VALID( device_.is_initialized( ) );
    LTB_CHECK_VALID( handle.is_valid( ) );
    LTB_CHECK( check_settings( info.settings ) );

    auto const surface = IOSurfacePtr{ ::IOSurfaceLookupFromMachPort( handle.get( ) ) };
    handle.reset( ); // The surface reference (if any) stays valid without the port.
    LTB_CHECK_VALID( nullptr != surface, "the handle isn't an IOSurface's Mach port" );

    LTB_CHECK_VALID(
        ( ::IOSurfaceGetWidth( surface.get( ) ) == info.settings.extent.width )
            && ( ::IOSurfaceGetHeight( surface.get( ) ) == info.settings.extent.height ),
        "the shared surface's size doesn't match the image settings"
    );

    auto const import_resources = [ this, &info, &surface ]( ) -> utils::Result< void > {
        LTB_CHECK( create_image( image_, info.settings, surface.get( ) ) );
        LTB_CHECK( bind_memory( device_, image_, memory_ ) );
        info_ = info;
        return utils::success( );
    };

    if ( auto result = import_resources( ); !result )
    {
        this->reset( );
        return result;
    }
    initialized_ = true;
    return utils::success( );
}

auto SharedImage::export_handle( ) const -> utils::Result< ipc::NativeHandle >
{
    LTB_CHECK_VALID( this->is_initialized( ) );
    LTB_CHECK( auto const surface, backing_surface( device_, image_ ) );

    auto handle = ipc::NativeHandle{ ::IOSurfaceCreateMachPort( surface ) };
    LTB_CHECK_VALID( handle.is_valid( ), "IOSurfaceCreateMachPort failed" );
    return handle;
}

auto SharedImage::is_initialized( ) const -> bool
{
    return initialized_;
}

auto SharedImage::info( ) const -> SharedImageInfo const&
{
    return info_;
}

auto SharedImage::image( ) const -> Image const&
{
    return image_;
}

auto SharedImage::image( ) -> Image&
{
    return image_;
}

auto SharedImage::reset( ) -> void
{
    memory_.reset( );
    image_.reset( );
    info_        = { };
    initialized_ = false;
}

} // namespace ltb::vlk

#endif // defined( __APPLE__ )
