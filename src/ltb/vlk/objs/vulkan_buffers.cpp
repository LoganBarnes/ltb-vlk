// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/vlk/objs/vulkan_buffers.hpp"

// project
#include "ltb/vlk/check.hpp"
#include "ltb/vlk/device_memory_utils.hpp"
#include "ltb/vlk/vector_utils.hpp"

namespace ltb::vlk::objs
{

VulkanBuffers::VulkanBuffers( VulkanGpu& gpu )
    : gpu_( gpu )
{
}

auto VulkanBuffers::initialize( VulkanBuffersSettings const& settings ) -> utils::Result< void >
{
    if ( this->is_initialized( ) )
    {
        return utils::success( );
    }
    LTB_CHECK_VALID( gpu_.is_initialized( ) );
    LTB_CHECK_VALID( !settings.buffers.empty( ) );

    for ( auto const& buffer_settings : settings.buffers )
    {
        LTB_CHECK( buffers_.emplace_back( gpu_.device( ) ).initialize( buffer_settings ) );
    }

    auto const memory_requirements = get_member_functions( buffers_, &Buffer::memory_requirements );

    LTB_CHECK(
        layout_,
        initialize_memory_chunk(
            memory_,
            gpu_.physical_device( ),
            memory_requirements,
            settings.memory_properties
        )
    );
    LTB_CHECK_VALID( layout_.ranges.size( ) == buffers_.size( ) );

    auto const& device = gpu_.device( ).get( );

    auto const buffer_count = buffers_.size( );
    for ( auto i = 0U; i < buffer_count; ++i )
    {
        auto const& buffer       = buffers_[ i ];
        auto const& memory_range = layout_.ranges[ i ];

        VK_CHECK( device.bindBufferMemory( buffer.get( ), memory_.get( ), memory_range.offset ) );
    }

    initialized_ = true;

    return utils::success( );
}

auto VulkanBuffers::is_initialized( ) const -> bool
{
    return initialized_;
}

auto VulkanBuffers::reset( ) -> void
{
    memory_.reset( );
    layout_ = { };
    buffers_.clear( );
    initialized_ = false;
}

auto VulkanBuffers::layout( ) const -> MemoryLayout const&
{
    return layout_;
}

auto VulkanBuffers::layout( ) -> MemoryLayout&
{
    return layout_;
}

auto VulkanBuffers::buffers( ) const -> std::vector< Buffer > const&
{
    return buffers_;
}

auto VulkanBuffers::memory( ) const -> DeviceMemory const&
{
    return memory_;
}

auto VulkanBuffers::memory( ) -> DeviceMemory&
{
    return memory_;
}

} // namespace ltb::vlk::objs
