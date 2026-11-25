// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
// SharedImage for platforms that share memory with Vulkan external memory handles (Linux
// and Windows). The platform-specific parts are in shared_image_<platform>.cpp, behind
// shared_image_backend.hpp.
#include "ltb/vlk/shared_image.hpp"

#if defined( LTB_IPC_SUPPORTED )

// project
#include "shared_image_backend.hpp"
#include "ltb/vlk/check.hpp"
#include "ltb/vlk/device.hpp"
#include "ltb/vlk/physical_device.hpp"

namespace ltb::vlk
{
namespace
{

auto const handle_type = detail::external_memory_handle_type( );

auto image_settings( SharedImageSettings const& settings ) -> ImageSettings
{
    return {
        .extent                       = { settings.extent.width, settings.extent.height, 1U },
        .format                       = settings.format,
        .usage                        = settings.usage,
        .external_memory_handle_types = handle_type,
    };
}

auto check_settings( SharedImageSettings const& settings ) -> utils::Result< void >
{
    LTB_CHECK_VALID( ( settings.extent.width > 0U ) && ( settings.extent.height > 0U ) );
    LTB_CHECK_VALID( vk::Format::eUndefined != settings.format );
    LTB_CHECK_VALID( static_cast< bool >( settings.usage ) );
    return utils::success( );
}

} // namespace

auto shared_image_device_extensions( ) -> std::vector< char const* >
{
    return detail::external_memory_device_extensions( );
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
        LTB_CHECK( image_.initialize( image_settings( settings ) ) );

        auto const requirements = image_.memory_requirements( );
        LTB_CHECK(
            auto const memory_type_index,
            device_.physical_device( ).find_memory_type_index(
                { requirements },
                vk::MemoryPropertyFlagBits::eDeviceLocal
            )
        );

        // A dedicated allocation, since some drivers require one for shared image memory
        // and both sides have to agree.
        LTB_CHECK( memory_.initialize( {
            .allocation_size     = requirements.size,
            .memory_type_index   = memory_type_index,
            .export_handle_types = handle_type,
            .dedicated_image     = image_.get( ),
        } ) );
        VK_CHECK( device_.get( ).bindImageMemory( image_.get( ), memory_.get( ), 0U ) );

        info_ = {
            .settings          = settings,
            .memory_type_index = memory_type_index,
            .allocation_size   = requirements.size,
        };
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

    auto const import_resources = [ this, &info, &handle ]( ) -> utils::Result< void > {
        // Identical settings to the exporter's image, so the memory's layout matches.
        LTB_CHECK( image_.initialize( image_settings( info.settings ) ) );

        auto const requirements = image_.memory_requirements( );
        LTB_CHECK_VALID(
            ( requirements.memoryTypeBits >> info.memory_type_index ) & 1U,
            "the exported memory type can't be used for this image"
        );
        LTB_CHECK_VALID( requirements.size <= info.allocation_size );

        LTB_CHECK(
            detail::import_memory(
                memory_,
                {
                    .allocation_size   = info.allocation_size,
                    .memory_type_index = info.memory_type_index,
                    .dedicated_image   = image_.get( ),
                },
                handle
            )
        );

        VK_CHECK( device_.get( ).bindImageMemory( image_.get( ), memory_.get( ), 0U ) );

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
    return detail::export_memory( device_, memory_ );
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

#endif // defined( LTB_IPC_SUPPORTED )
