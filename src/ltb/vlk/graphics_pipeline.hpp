// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/utils/result.hpp"
#include "ltb/vlk/fwd.hpp"
#include "ltb/vlk/vulkan.hpp"

// standard
#include <optional>

namespace ltb::vlk
{

struct GraphicsPipelineSettings
{
    std::vector< vk::DynamicState > dynamic_states = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
    };

    std::vector< vk::VertexInputBindingDescription >   vertex_bindings   = { };
    std::vector< vk::VertexInputAttributeDescription > vertex_attributes = { };

    vk::PrimitiveTopology primitive_topology        = vk::PrimitiveTopology::eTriangleList;
    bool                  primitive_restart_enabled = false;

    std::vector< vk::Viewport > viewports = { };
    std::vector< vk::Rect2D >   scissors  = { };

    std::optional< vk::PipelineViewportStateCreateInfo > viewport_state
        = vk::PipelineViewportStateCreateInfo{ }.setViewportCount( 1 ).setScissorCount( 1 );

    vk::PipelineRasterizationStateCreateInfo rasterizer
        = vk::PipelineRasterizationStateCreateInfo{ }
              .setDepthClampEnable( false )
              .setRasterizerDiscardEnable( false )
              .setPolygonMode( vk::PolygonMode::eFill )
              .setLineWidth( 1.0F )
              .setCullMode( vk::CullModeFlagBits::eBack )
              .setFrontFace( vk::FrontFace::eCounterClockwise )
              .setDepthBiasEnable( false );

    vk::PipelineMultisampleStateCreateInfo multisampling
        = vk::PipelineMultisampleStateCreateInfo{ }
              .setSampleShadingEnable( false )
              .setRasterizationSamples( vk::SampleCountFlagBits::e1 );

    std::optional< vk::PipelineDepthStencilStateCreateInfo > depth_stencil
        = vk::PipelineDepthStencilStateCreateInfo{ }
              .setDepthTestEnable( true )
              .setDepthWriteEnable( true )
              .setDepthCompareOp( vk::CompareOp::eLess )
              .setDepthBoundsTestEnable( false )
              .setStencilTestEnable( false );

    std::optional< vk::PipelineTessellationStateCreateInfo > tessellation_state = std::nullopt;
};

class GraphicsPipeline
{
public:
    GraphicsPipeline(
        Device&                      device,
        RenderPass&                  render_pass,
        std::vector< ShaderModule >& shader_modules,
        PipelineLayout&              pipeline_layout
    );

    auto initialize( GraphicsPipelineSettings settings ) -> utils::Result< void >;

    [[nodiscard( "Const getter" )]]
    auto is_initialized( ) const -> bool;

    [[nodiscard( "Const getter" )]]
    auto get( ) const -> vk::Pipeline const&;
    auto get( ) -> vk::Pipeline&;

    auto reset( ) -> void;

    [[nodiscard( "Const getter" )]]
    auto settings( ) const -> GraphicsPipelineSettings const&;

private:
    Device&                      device_;
    RenderPass&                  render_pass_;
    std::vector< ShaderModule >& shader_modules_;
    PipelineLayout&              pipeline_layout_;

    GraphicsPipelineSettings settings_ = { };
    vk::UniquePipeline       pipeline_ = { };
};

} // namespace ltb::vlk
