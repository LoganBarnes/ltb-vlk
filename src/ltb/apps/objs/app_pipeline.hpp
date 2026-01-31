// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/vlk/fwd.hpp"
#include "ltb/vlk/objs/fwd.hpp"
#include "ltb/vlk/objs/vulkan_buffer.hpp"
#include "ltb/vlk/objs/vulkan_graphics_pipeline.hpp"

// external
#include <glm/gtc/matrix_transform.hpp>

// standard
#include <list>

namespace ltb
{
struct ObjsAppPipelineSettings
{
    uint32                   frame_count = 0U;
    vlk::objs::VulkanBuffer& camera_ubo;
};

class ObjsAppPipeline
{
public:
    using VertexPosition                      = glm::vec3;
    static constexpr auto position_size_bytes = sizeof( VertexPosition );
    static constexpr auto position_format     = vk::Format::eR32G32B32Sfloat;

    using VertexColor                      = glm::vec3;
    static constexpr auto color_size_bytes = sizeof( VertexColor );
    static constexpr auto color_format     = vk::Format::eR32G32B32Sfloat;

    using VertexIndex                      = uint32;
    static constexpr auto index_size_bytes = sizeof( VertexIndex );
    static constexpr auto index_type       = vk::IndexType::eUint32;

    struct TriangleMesh
    {
        std::vector< VertexPosition > positions = { };
        std::vector< VertexColor >    colors    = { };
        std::vector< VertexIndex >    indices   = { };
    };

    struct MeshPushConstants
    {
        glm::mat4 transform = glm::identity< glm::mat4 >( );
    };

    ObjsAppPipeline( vlk::objs::VulkanGpu& gpu, vlk::objs::VulkanPresentation& presentation );

    auto initialize( ObjsAppPipelineSettings settings ) -> utils::Result< void >;

    [[nodiscard( "Const getter" )]]
    auto is_initialized( ) const -> bool;

    auto initialize_mesh(
        TriangleMesh const& mesh,
        vlk::CommandPool&   command_pool,
        vk::Queue const&    queue
    ) -> utils::Result< MeshPushConstants* >;

    auto draw_meshes( vlk::objs::FrameInfo const& frame ) -> utils::Result< void >;

    [[nodiscard( "Const getter" )]]
    auto pipeline( ) const -> vlk::objs::VulkanGraphicsPipeline const&;
    auto pipeline( ) -> vlk::objs::VulkanGraphicsPipeline&;

private:
    vlk::objs::VulkanGpu&          gpu_;
    vlk::objs::VulkanPresentation& presentation_;

    vlk::objs::VulkanGraphicsPipeline pipeline_ = { gpu_, presentation_ };

    bool initialized_ = false;

    struct MeshAndUniforms
    {
        vlk::objs::VulkanBuffer vbo;
        uint32                  index_count    = 0U;
        MeshPushConstants       push_constants = { };

        explicit MeshAndUniforms( vlk::objs::VulkanGpu& gpu )
            : vbo( gpu )
        {
        }
    };

    std::list< MeshAndUniforms > mesh_data_ = { };

    auto draw_mesh( MeshAndUniforms const& mesh_data, vlk::objs::FrameInfo const& frame )
        -> utils::Result< void >;
};

} // namespace ltb
