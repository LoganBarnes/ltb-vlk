// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/vlk/compute_pipeline.hpp"

// project
#include "ltb/vlk/check.hpp"
#include "ltb/vlk/device.hpp"
#include "ltb/vlk/pipeline_layout.hpp"
#include "ltb/vlk/shader_module.hpp"

// external
#include <spdlog/spdlog.h>

namespace ltb::vlk
{

ComputePipeline::ComputePipeline(
    Device&         device,
    ShaderModule&   shader_module,
    PipelineLayout& pipeline_layout
)
    : device_( device )
    , shader_module_( shader_module )
    , pipeline_layout_( pipeline_layout )
{
}

auto ComputePipeline::initialize( ) -> utils::Result< void >
{
    if ( this->is_initialized( ) )
    {
        return utils::success( );
    }
    LTB_CHECK_VALID( device_.is_initialized( ) );
    LTB_CHECK_VALID( shader_module_.is_initialized( ) );
    LTB_CHECK_VALID( pipeline_layout_.is_initialized( ) );

    auto const shader_stage_info = shader_module_.get_shader_stage_create_info( );

    auto const compute_pipeline_info = vk::ComputePipelineCreateInfo{ }
                                           .setLayout( pipeline_layout_.get( ) )
                                           .setStage( shader_stage_info );

    VK_CHECK(
        auto pipeline,
        device_.get( ).createComputePipelineUnique( nullptr, compute_pipeline_info )
    );
    spdlog::debug( "vk::createComputePipelineUnique()" );

    pipeline_ = std::move( pipeline );

    return utils::success( );
}

auto ComputePipeline::is_initialized( ) const -> bool
{
    return nullptr != pipeline_.get( );
}

auto ComputePipeline::get( ) const -> vk::Pipeline const&
{
    return pipeline_.get( );
}

auto ComputePipeline::get( ) -> vk::Pipeline&
{
    return pipeline_.get( );
}

} // namespace ltb::vlk
