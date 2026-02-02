// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/vlk/dd/mesh_data.hpp"
#include "ltb/vlk/dd/simple_mesh_2.hpp"
#include "ltb/vlk/fwd.hpp"
#include "ltb/vlk/objs/fwd.hpp"
#include "ltb/vlk/objs/vulkan_buffer.hpp"
#include "ltb/vlk/objs/vulkan_graphics_pipeline.hpp"

// standard
#include <list>

namespace ltb::vlk::dd
{
struct LinesPipeline2Settings
{
    uint32                    frame_count = 0U;
    objs::VulkanBuffer const& camera_ubo;
};

class LinesPipeline2
{
public:
    LinesPipeline2( objs::VulkanGpu& gpu, objs::VulkanPresentation& presentation );

    auto initialize( LinesPipeline2Settings const& settings ) -> utils::Result< void >;

    [[nodiscard( "Const getter" )]]
    auto is_initialized( ) const -> bool;

    auto
    initialize_mesh( SimpleMesh2 const& mesh, CommandPool& command_pool, vk::Queue const& queue )
        -> utils::Result< SimpleMeshUniforms* >;

    auto draw( objs::FrameInfo const& frame ) -> utils::Result< void >;

private:
    objs::VulkanGpu&          gpu_;
    objs::VulkanPresentation& presentation_;

    objs::VulkanBuffer display_ubo_ = { gpu_ };

    objs::VulkanGraphicsPipeline pipeline_ = { gpu_, presentation_ };

    bool initialized_ = false;

    using MeshAndUniforms = MeshData< SimpleMeshUniforms >;

    std::list< MeshAndUniforms > mesh_data_ = { };

    auto draw_mesh( MeshAndUniforms const& mesh_data, objs::FrameInfo const& frame )
        -> utils::Result< void >;
};

} // namespace ltb::vlk::dd
