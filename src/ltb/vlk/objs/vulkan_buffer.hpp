// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/vlk/buffer.hpp"
#include "ltb/vlk/device_memory.hpp"
#include "ltb/vlk/objs/vulkan_gpu.hpp"

namespace ltb::vlk::objs
{

struct VulkanBufferSettings
{
    MemoryLayout layout = { };

    vk::BufferCreateFlags buffer_flags        = { };
    vk::BufferUsageFlags  buffer_usage        = vk::BufferUsageFlagBits::eVertexBuffer;
    vk::SharingMode       buffer_sharing_mode = vk::SharingMode::eExclusive;

    vk::MemoryPropertyFlags memory_properties = { };

    bool store_mapped_value = false;
};

class VulkanBuffer
{
public:
    explicit( false ) VulkanBuffer( VulkanGpu& gpu );

    auto initialize( VulkanBufferSettings settings ) -> utils::Result< void >;

    [[nodiscard( "Const getter" )]]
    auto is_initialized( ) const -> bool;

    auto reset( ) -> void;

    [[nodiscard( "Const getter" )]]
    auto layout( ) const -> MemoryLayout const&;
    auto layout( ) -> MemoryLayout&;

    [[nodiscard( "Const getter" )]]
    auto buffer( ) const -> Buffer const&;
    auto buffer( ) -> Buffer&;

    [[nodiscard( "Const getter" )]]
    auto memory( ) const -> DeviceMemory const&;
    auto memory( ) -> DeviceMemory&;

    [[nodiscard( "Const getter" )]]
    auto mapped_data( ) const -> uint8*;

private:
    VulkanGpu& gpu_;

    MemoryLayout layout_      = { };
    Buffer       buffer_      = { gpu_.device( ) };
    DeviceMemory memory_      = { gpu_.device( ) };
    uint8*       mapped_data_ = nullptr;

    bool initialized_ = false;
};

} // namespace ltb::vlk::objs
