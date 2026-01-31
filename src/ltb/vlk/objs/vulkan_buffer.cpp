// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/vlk/objs/vulkan_buffer.hpp"

// project
#include "ltb/vlk/check.hpp"
#include "ltb/vlk/device_memory_utils.hpp"

namespace ltb::vlk::objs
{

VulkanBuffer::VulkanBuffer( VulkanGpu& gpu )
    : gpu_( gpu )
{
}

auto VulkanBuffer::initialize( VulkanBufferSettings settings ) -> utils::Result< void >
{
    if ( this->is_initialized( ) )
    {
        return utils::success( );
    }
    LTB_CHECK_VALID( gpu_.is_initialized( ) );

    LTB_CHECK( buffer_.initialize( {
        .flags        = settings.buffer_flags,
        .size         = settings.layout.total_size,
        .usage        = settings.buffer_usage,
        .sharing_mode = settings.buffer_sharing_mode,
    } ) );

    LTB_CHECK(
        auto const memory_range,
        initialize_memory_chunk(
            memory_,
            gpu_.physical_device( ),
            buffer_.memory_requirements( ),
            settings.memory_properties
        )
    );
    LTB_CHECK_VALID( settings.layout.total_size <= memory_range.size );

    VK_CHECK( gpu_.device( )
                  .get( )
                  .bindBufferMemory( buffer_.get( ), memory_.get( ), memory_range.offset ) );

    if ( settings.store_mapped_value )
    {
        LTB_CHECK( mapped_data_, memory_.map( memory_range, { } ) );
    }

    layout_ = std::move( settings.layout );

    initialized_ = true;

    return utils::success( );
}

auto VulkanBuffer::is_initialized( ) const -> bool
{
    return initialized_;
}

auto VulkanBuffer::reset( ) -> void
{
    memory_.reset( );
    buffer_.reset( );
    layout_      = { };
    initialized_ = false;
}

auto VulkanBuffer::layout( ) const -> MemoryLayout const&
{
    return layout_;
}

auto VulkanBuffer::layout( ) -> MemoryLayout&
{
    return layout_;
}

auto VulkanBuffer::buffer( ) const -> Buffer const&
{
    return buffer_;
}

auto VulkanBuffer::buffer( ) -> Buffer&
{
    return buffer_;
}

auto VulkanBuffer::memory( ) const -> DeviceMemory const&
{
    return memory_;
}

auto VulkanBuffer::memory( ) -> DeviceMemory&
{
    return memory_;
}

auto VulkanBuffer::mapped_data( ) const -> uint8*
{
    return mapped_data_;
}

} // namespace ltb::vlk::objs
