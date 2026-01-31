// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/vlk/device_memory_utils.hpp"

// project
#include "ltb/vlk/check.hpp"
#include "ltb/vlk/device.hpp"
#include "ltb/vlk/physical_device.hpp"
#include "ltb/vlk/vector_utils.hpp"

// external
#include <spdlog/spdlog.h>

namespace ltb::vlk
{

auto initialize_memory_chunk(
    DeviceMemory&                 memory,
    PhysicalDevice const&         physical_device,
    vk::MemoryRequirements const& memory_requirements,
    vk::MemoryPropertyFlags const memory_properties
) -> utils::Result< MemoryRange >
{
    LTB_CHECK(
        auto const memory_layout,
        initialize_memory_chunk(
            memory,
            physical_device,
            std::vector{ memory_requirements },
            memory_properties
        )
    );

    LTB_CHECK_VALID( 1UZ == memory_layout.ranges.size( ) );

    return memory_layout.ranges.front( );
}

auto initialize_memory_chunk(
    DeviceMemory&                                memory,
    PhysicalDevice const&                        physical_device,
    std::vector< vk::MemoryRequirements > const& memory_requirements,
    vk::MemoryPropertyFlags const                memory_properties
) -> utils::Result< MemoryLayout >
{
    auto memory_layout = compute_memory_layout( memory_requirements );

    LTB_CHECK(
        auto const vertex_memory_type_index,
        physical_device.find_memory_type_index( memory_requirements, memory_properties )
    );

    LTB_CHECK( memory.initialize( {
        .allocation_size   = memory_layout.total_size,
        .memory_type_index = vertex_memory_type_index,
    } ) );

    return memory_layout;
}

auto initialize_buffer_memory(
    DeviceMemory&                 memory,
    Device const&                 device,
    Buffer const&                 buffer,
    vk::MemoryPropertyFlags const memory_properties
) -> utils::Result< MemoryRange >
{
    auto const memory_requirements = std::vector{ buffer.memory_requirements( ) };

    LTB_CHECK(
        auto const memory_layout,
        initialize_memory_chunk(
            memory,
            device.physical_device( ),
            memory_requirements,
            memory_properties
        )
    );

    LTB_CHECK_VALID( 1UZ == memory_layout.ranges.size( ) );

    auto const& memory_range = memory_layout.ranges.front( );

    VK_CHECK( device.get( ).bindBufferMemory( buffer.get( ), memory.get( ), memory_range.offset ) );

    return memory_range;
}

auto initialize_buffers_memory(
    DeviceMemory&                                          memory,
    Device const&                                          device,
    std::vector< std::reference_wrapper< Buffer > > const& buffers,
    vk::MemoryPropertyFlags const                          memory_properties
) -> utils::Result< MemoryLayout >
{
    auto const memory_requirements = get_member_functions( buffers, &Buffer::memory_requirements );

    LTB_CHECK(
        auto memory_layout,
        initialize_memory_chunk(
            memory,
            device.physical_device( ),
            memory_requirements,
            memory_properties
        )
    );

    auto const buffer_count = buffers.size( );
    LTB_CHECK_VALID( buffer_count == memory_layout.ranges.size( ) );
    for ( auto i = 0U; i < buffer_count; ++i )
    {
        auto const& buffer       = buffers[ i ].get( );
        auto const& memory_range = memory_layout.ranges[ i ];

        VK_CHECK(
            device.get( ).bindBufferMemory( buffer.get( ), memory.get( ), memory_range.offset )
        );
    }

    return memory_layout;
}

auto initialize_image_memory(
    DeviceMemory&                 memory,
    Device const&                 device,
    Image const&                  image,
    vk::MemoryPropertyFlags const memory_properties
) -> utils::Result< MemoryRange >
{
    auto const memory_requirements = std::vector{ image.memory_requirements( ) };

    LTB_CHECK(
        auto const memory_layout,
        initialize_memory_chunk(
            memory,
            device.physical_device( ),
            memory_requirements,
            memory_properties
        )
    );

    LTB_CHECK_VALID( 1UZ == memory_layout.ranges.size( ) );
    auto const& memory_range = memory_layout.ranges.front( );

    VK_CHECK( device.get( ).bindImageMemory( image.get( ), memory.get( ), memory_range.offset ) );

    return memory_range;
}

auto initialize_images_memory(
    DeviceMemory&                                         memory,
    Device const&                                         device,
    std::vector< std::reference_wrapper< Image > > const& images,
    vk::MemoryPropertyFlags const                         memory_properties
) -> utils::Result< MemoryLayout >
{
    auto const memory_requirements = get_member_functions( images, &Image::memory_requirements );

    LTB_CHECK(
        auto memory_layout,
        initialize_memory_chunk(
            memory,
            device.physical_device( ),
            memory_requirements,
            memory_properties
        )
    );

    auto const image_count = images.size( );
    LTB_CHECK_VALID( image_count == memory_layout.ranges.size( ) );

    for ( auto i = 0U; i < image_count; ++i )
    {
        auto const& image        = images[ i ].get( );
        auto const& memory_range = memory_layout.ranges[ i ];

        VK_CHECK(
            device.get( ).bindImageMemory( image.get( ), memory.get( ), memory_range.offset )
        );
    }

    return memory_layout;
}

auto append_memory_size(
    MemoryLayout&                 memory_layout,
    vk::MemoryRequirements const& memory_requirements
) -> void
{
    memory_layout.ranges.push_back( {
        .size   = memory_requirements.size,
        .offset = memory_layout.total_size,
    } );
    memory_layout.total_size += memory_layout.ranges.back( ).size;

    if ( 0U != memory_requirements.alignment )
    {
        auto const alignment_remainder = memory_layout.total_size % memory_requirements.alignment;
        memory_layout.total_size += ( memory_requirements.alignment - alignment_remainder );
    }
}

auto append_memory_size_n(
    MemoryLayout&                 memory_layout,
    vk::MemoryRequirements const& requirements,
    uint32 const                  count
) -> void
{
    memory_layout = { };
    memory_layout.ranges.reserve( count );

    for ( auto i = 0U; i < count; ++i )
    {
        append_memory_size( memory_layout, requirements );
    }
}

} // namespace ltb::vlk
