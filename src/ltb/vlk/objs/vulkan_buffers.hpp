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

struct VulkanBuffersSettings
{
    std::vector< BufferSettings > buffers           = { };
    vk::MemoryPropertyFlags       memory_properties = { };
};

class VulkanBuffers
{
public:
    explicit( false ) VulkanBuffers( VulkanGpu& gpu );

    auto initialize( VulkanBuffersSettings const& settings ) -> utils::Result< void >;

    [[nodiscard( "Const getter" )]]
    auto is_initialized( ) const -> bool;

    auto reset( ) -> void;

    [[nodiscard( "Const getter" )]]
    auto layout( ) const -> MemoryLayout const&;
    auto layout( ) -> MemoryLayout&;

    [[nodiscard( "Const getter" )]]
    auto buffers( ) const -> std::vector< Buffer > const&;

    [[nodiscard( "Const getter" )]]
    auto memory( ) const -> DeviceMemory const&;
    auto memory( ) -> DeviceMemory&;

private:
    VulkanGpu& gpu_;

    std::vector< Buffer > buffers_ = { };
    MemoryLayout          layout_  = { };
    DeviceMemory          memory_  = { gpu_.device( ) };

    bool initialized_ = false;
};

} // namespace ltb::vlk::objs
