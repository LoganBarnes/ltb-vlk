// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/vlk/buffer.hpp"
#include "ltb/vlk/device_memory.hpp"
#include "ltb/vlk/image.hpp"

namespace ltb::vlk
{

auto initialize_memory_chunk(
    DeviceMemory&                 memory,
    PhysicalDevice const&         physical_device,
    vk::MemoryRequirements const& memory_requirements,
    vk::MemoryPropertyFlags       memory_properties
) -> utils::Result< MemoryRange >;

auto initialize_memory_chunk(
    DeviceMemory&                                memory,
    PhysicalDevice const&                        physical_device,
    std::vector< vk::MemoryRequirements > const& memory_requirements,
    vk::MemoryPropertyFlags                      memory_properties
) -> utils::Result< MemoryLayout >;

auto initialize_buffer_memory(
    DeviceMemory&           memory,
    Device const&           device,
    Buffer const&           buffer,
    vk::MemoryPropertyFlags memory_properties
) -> utils::Result< MemoryRange >;

auto initialize_buffers_memory(
    DeviceMemory&                                          memory,
    Device const&                                          device,
    std::vector< std::reference_wrapper< Buffer > > const& buffers,
    vk::MemoryPropertyFlags                                memory_properties
) -> utils::Result< MemoryLayout >;

auto initialize_image_memory(
    DeviceMemory&           memory,
    Device const&           device,
    Image const&            image,
    vk::MemoryPropertyFlags memory_properties
) -> utils::Result< MemoryRange >;

auto initialize_images_memory(
    DeviceMemory&                                         memory,
    Device const&                                         device,
    std::vector< std::reference_wrapper< Image > > const& images,
    vk::MemoryPropertyFlags                               memory_properties
) -> utils::Result< MemoryLayout >;

auto append_memory_size(
    MemoryLayout&                 memory_layout,
    vk::MemoryRequirements const& memory_requirements
) -> void;

auto append_memory_size_n(
    MemoryLayout&                 memory_layout,
    vk::MemoryRequirements const& memory_requirements,
    uint32                        count
) -> void;

struct BufferAndMemory
{
    Device& device;

    Buffer       buffer = { device };
    DeviceMemory memory = { device };
    MemoryLayout layout = { };
};

struct BuffersAndMemory
{
    Device& device;

    std::vector< Buffer > buffers = { };
    DeviceMemory          memory  = { device };
    MemoryLayout          layout  = { };
};

template < typename BufAndMemObj >
auto initialize_buffer_and_memory(
    BufAndMemObj&           buf_and_mem,
    Device const&           device,
    vk::DeviceSize          size,
    vk::BufferUsageFlags    buffer_usage,
    vk::MemoryPropertyFlags memory_properties
) -> utils::Result< MemoryRange >
{
    LTB_CHECK( buf_and_mem.buffer.initialize( {
        .size  = size,
        .usage = buffer_usage,
    } ) );

    LTB_CHECK(
        auto const memory_range,
        vlk::initialize_buffer_memory(
            buf_and_mem.memory,
            device,
            buf_and_mem.buffer,
            memory_properties
        )
    );

    return memory_range;
}

} // namespace ltb::vlk
