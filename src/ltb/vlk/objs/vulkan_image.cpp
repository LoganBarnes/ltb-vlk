// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/vlk/objs/vulkan_image.hpp"

// project
#include "ltb/vlk/check.hpp"
#include "ltb/vlk/device_memory_utils.hpp"

namespace ltb::vlk::objs
{

VulkanImage::VulkanImage( VulkanGpu& gpu )
    : gpu_( gpu )
{
}

auto VulkanImage::initialize( VulkanImageSettings settings ) -> utils::Result< void >
{
    if ( this->is_initialized( ) )
    {
        return utils::success( );
    }
    LTB_CHECK_VALID( gpu_.is_initialized( ) );

    LTB_CHECK( image_.initialize( std::move( settings.image ) ) );

    LTB_CHECK(
        auto const memory_range,
        initialize_memory_chunk(
            memory_,
            gpu_.physical_device( ),
            image_.memory_requirements( ),
            settings.memory_properties
        )
    );

    VK_CHECK(
        gpu_.device( ).get( ).bindImageMemory( image_.get( ), memory_.get( ), memory_range.offset )
    );

    initialized_ = true;

    return utils::success( );
}

auto VulkanImage::is_initialized( ) const -> bool
{
    return initialized_;
}

auto VulkanImage::reset( ) -> void
{
    memory_.reset( );
    image_.reset( );
    initialized_ = false;
}

auto VulkanImage::image( ) const -> Image const&
{
    return image_;
}

auto VulkanImage::image( ) -> Image&
{
    return image_;
}

auto VulkanImage::memory( ) const -> DeviceMemory const&
{
    return memory_;
}

auto VulkanImage::memory( ) -> DeviceMemory&
{
    return memory_;
}

} // namespace ltb::vlk::objs
