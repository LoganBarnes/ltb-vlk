// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/vlk/graphics_pipeline.hpp"

// project
#include "ltb/vlk/check.hpp"
#include "ltb/vlk/device.hpp"
#include "ltb/vlk/pipeline_layout.hpp"
#include "ltb/vlk/render_pass.hpp"
#include "ltb/vlk/shader_module.hpp"

// external
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/transform.hpp>
#include <spdlog/spdlog.h>

namespace ltb::vlk
{

GraphicsPipeline::GraphicsPipeline(
    Device&                      device,
    RenderPass&                  render_pass,
    std::vector< ShaderModule >& shader_modules,
    PipelineLayout&              pipeline_layout
)
    : device_( device )
    , render_pass_( render_pass )
    , shader_modules_( shader_modules )
    , pipeline_layout_( pipeline_layout )
{
}

auto GraphicsPipeline::initialize( GraphicsPipelineSettings settings ) -> utils::Result< void >
{
    if ( this->is_initialized( ) )
    {
        return utils::success( );
    }
    LTB_CHECK_VALID( device_.is_initialized( ) );
    LTB_CHECK_VALID( render_pass_.is_initialized( ) );
    LTB_CHECK_VALID( pipeline_layout_.is_initialized( ) );
    for ( auto& shader_module : shader_modules_ )
    {
        LTB_CHECK_VALID( shader_module.is_initialized( ) );
    }

    if ( settings.viewport_state
         && ( ( !settings.viewports.empty( ) ) || ( !settings.scissors.empty( ) ) ) )
    {
        return LTB_MAKE_UNEXPECTED_ERROR(
            "viewport_state should not be used when viewports "
            "and scissors are provided. Use one or the other."
        );
    }

    auto const shader_stages = shader_modules_
                             | ranges::views::transform( GetShaderStageCreateInfo{ } )
                             | ranges::to< std::vector >( );

    auto const vertex_input_info
        = vk::PipelineVertexInputStateCreateInfo{ }
              .setVertexBindingDescriptions( settings.vertex_bindings )
              .setVertexAttributeDescriptions( settings.vertex_attributes );

    auto const input_assembly
        = vk::PipelineInputAssemblyStateCreateInfo{ }
              .setTopology( settings.primitive_topology )
              .setPrimitiveRestartEnable( settings.primitive_restart_enabled );

    auto const viewport_state = settings.viewport_state.value_or(
        vk::PipelineViewportStateCreateInfo{ }
            .setViewports( settings.viewports )
            .setScissors( settings.scissors )
    );

    auto const color_blend_attachments = std::vector{
        vk::PipelineColorBlendAttachmentState{ }
            .setColorWriteMask(
                vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
                | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
            )
            .setBlendEnable( false ),
    };

    auto const color_blending = vk::PipelineColorBlendStateCreateInfo{ }
                                    .setLogicOpEnable( false )
                                    .setLogicOp( vk::LogicOp::eCopy )
                                    .setAttachments( color_blend_attachments )
                                    .setBlendConstants( { 0.0F, 0.0F, 0.0F, 0.0F } );

    auto const dynamic_state
        = vk::PipelineDynamicStateCreateInfo{ }.setDynamicStates( settings.dynamic_states );

    vk::PipelineDepthStencilStateCreateInfo const* depth_stencil_ptr = nullptr;
    if ( settings.depth_stencil )
    {
        depth_stencil_ptr = &settings.depth_stencil.value( );
    }

    vk::PipelineTessellationStateCreateInfo const* tessellation_state_ptr = nullptr;
    if ( settings.tessellation_state.has_value( ) )
    {
        tessellation_state_ptr = &settings.tessellation_state.value( );
    }

    auto const pipeline_infos = std::vector{
        vk::GraphicsPipelineCreateInfo{ }
            .setStages( shader_stages )
            .setPVertexInputState( &vertex_input_info )
            .setPInputAssemblyState( &input_assembly )
            .setPTessellationState( tessellation_state_ptr )
            .setPViewportState( &viewport_state )
            .setPRasterizationState( &settings.rasterizer )
            .setPMultisampleState( &settings.multisampling )
            .setPDepthStencilState( depth_stencil_ptr )
            .setPColorBlendState( &color_blending )
            .setPDynamicState( &dynamic_state )
            .setLayout( pipeline_layout_.get( ) )
            .setRenderPass( render_pass_.get( ) )
            .setSubpass( 0 ),
    };

    VK_CHECK(
        auto pipelines,
        device_.get( ).createGraphicsPipelinesUnique( nullptr, pipeline_infos )
    );
    spdlog::debug( "vk::createGraphicsPipelinesUnique()" );

    settings_ = std::move( settings );
    pipeline_ = std::move( pipelines.front( ) );

    return utils::success( );
}

auto GraphicsPipeline::is_initialized( ) const -> bool
{
    return nullptr != pipeline_.get( );
}

auto GraphicsPipeline::get( ) const -> vk::Pipeline const&
{
    return pipeline_.get( );
}

auto GraphicsPipeline::get( ) -> vk::Pipeline&
{
    return pipeline_.get( );
}

auto GraphicsPipeline::reset( ) -> void
{
    pipeline_.reset( );
    settings_ = { };
}

auto GraphicsPipeline::settings( ) const -> GraphicsPipelineSettings const&
{
    return settings_;
}

} // namespace ltb::vlk
