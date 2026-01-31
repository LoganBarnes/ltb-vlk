// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/vlk/objs/vulkan_graphics_pipeline.hpp"

// project
#include "ltb/vlk/check.hpp"
#include "ltb/vlk/objs/frame_info.hpp"
#include "ltb/vlk/vector_utils.hpp"

// external
#include <spdlog/spdlog.h>

namespace ltb::vlk::objs
{

VulkanGraphicsPipeline::VulkanGraphicsPipeline( VulkanGpu& gpu, VulkanPresentation& presentation )
    : gpu_( gpu )
    , presentation_( presentation )
{
}

auto VulkanGraphicsPipeline::initialize( VulkanGraphicsPipelineSettings settings )
    -> utils::Result< void >
{
    if ( this->is_initialized( ) )
    {
        return utils::success( );
    }
    LTB_CHECK_VALID( gpu_.is_initialized( ) );
    LTB_CHECK_VALID( presentation_.is_initialized( ) );

    LTB_CHECK_VALID( !settings.shader_modules.empty( ) );
    LTB_CHECK_VALID( settings.descriptor_set_count > 0U );

    for ( auto& shader_module_settings : settings.shader_modules )
    {
        LTB_CHECK( shader_modules_.emplace_back( gpu_.device( ) )
                       .initialize( std::move( shader_module_settings ) ) );
    }

    descriptor_set_layouts_.reserve( settings.uniform_binding_sets.size( ) );
    descriptor_sets_.reserve( settings.uniform_binding_sets.size( ) );

    for ( auto& bindings : settings.uniform_binding_sets )
    {
        LTB_CHECK( descriptor_set_layouts_.emplace_back( gpu_.device( ) )
                       .initialize( {
                           .bindings = std::move( bindings ),
                       } ) );

        LTB_CHECK( descriptor_sets_.emplace_back( gpu_.device( ), gpu_.descriptor_pool( ) )
                       .initialize( {
                           .layouts = {
                               settings.descriptor_set_count,
                               descriptor_set_layouts_.back( ).get( ),
                           },
                       } ) );
    }

    auto descriptor_set_layouts = descriptor_set_layouts_ | ranges::views::transform( Get{ } )
                                | ranges::to< std::vector >( );

    LTB_CHECK( pipeline_layout_.initialize( {
        .descriptor_set_layouts = std::move( descriptor_set_layouts ),
        .push_constant_ranges   = std::move( settings.uniform_push_constants ),
    } ) );

    LTB_CHECK( pipeline_.initialize( std::move( settings.pipeline ) ) );

    initialized_ = true;

    return utils::success( );
}

auto VulkanGraphicsPipeline::bind( vk::CommandBuffer const& command_buffer ) -> void
{
    command_buffer.bindPipeline( vk::PipelineBindPoint::eGraphics, pipeline_.get( ) );
}

auto VulkanGraphicsPipeline::bind_descriptor_sets( FrameInfo const& frame ) -> utils::Result< void >
{
    for ( auto& descriptor_sets : descriptor_sets_ )
    {
        auto const descriptors = descriptor_sets.get( );
        LTB_CHECK_VALID( frame.frame_index < descriptors.size( ) );

        constexpr auto bind_point = vk::PipelineBindPoint::eGraphics;
        constexpr auto first_set  = 0U;
        frame.command_buffer.bindDescriptorSets(
            bind_point,
            pipeline_layout_.get( ),
            first_set,
            descriptors[ frame.frame_index ],
            { }
        );
    }

    return utils::success( );
}

auto VulkanGraphicsPipeline::is_initialized( ) const -> bool
{
    return initialized_;
}

auto VulkanGraphicsPipeline::shader_modules( ) const -> std::vector< ShaderModule > const&
{
    return shader_modules_;
}

auto VulkanGraphicsPipeline::shader_modules( ) -> std::vector< ShaderModule >&
{
    return shader_modules_;
}

auto VulkanGraphicsPipeline::descriptor_set_layouts( ) const
    -> std::vector< DescriptorSetLayout > const&
{
    return descriptor_set_layouts_;
}

auto VulkanGraphicsPipeline::descriptor_set_layouts( ) -> std::vector< DescriptorSetLayout >&
{
    return descriptor_set_layouts_;
}

auto VulkanGraphicsPipeline::descriptor_sets( ) const -> std::vector< DescriptorSets > const&
{
    return descriptor_sets_;
}

auto VulkanGraphicsPipeline::descriptor_sets( ) -> std::vector< DescriptorSets >&
{
    return descriptor_sets_;
}

auto VulkanGraphicsPipeline::pipeline_layout( ) const -> PipelineLayout const&
{
    return pipeline_layout_;
}

auto VulkanGraphicsPipeline::pipeline_layout( ) -> PipelineLayout&
{
    return pipeline_layout_;
}

auto VulkanGraphicsPipeline::pipeline( ) const -> GraphicsPipeline const&
{
    return pipeline_;
}

auto VulkanGraphicsPipeline::pipeline( ) -> GraphicsPipeline&
{
    return pipeline_;
}

} // namespace ltb::vlk::objs
