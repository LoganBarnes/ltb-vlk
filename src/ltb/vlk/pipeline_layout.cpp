// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/vlk/pipeline_layout.hpp"

// project
#include "ltb/vlk/check.hpp"
#include "ltb/vlk/device.hpp"

// external
#include <spdlog/spdlog.h>

namespace ltb::vlk
{

PipelineLayout::PipelineLayout( Device& device )
    : device_( device )
{
}

auto PipelineLayout::initialize( PipelineLayoutSettings settings ) -> utils::Result< void >
{
    if ( this->is_initialized( ) )
    {
        return utils::success( );
    }
    LTB_CHECK_VALID( device_.is_initialized( ) );

    auto const pipeline_layout_info = vk::PipelineLayoutCreateInfo{ }
                                          .setSetLayouts( settings.descriptor_set_layouts )
                                          .setPushConstantRanges( settings.push_constant_ranges );
    VK_CHECK(
        auto pipeline_layout,
        device_.get( ).createPipelineLayoutUnique( pipeline_layout_info )
    );
    spdlog::debug( "vk::createPipelineLayout()" );

    settings_        = std::move( settings );
    pipeline_layout_ = std::move( pipeline_layout );

    return utils::success( );
}

auto PipelineLayout::is_initialized( ) const -> bool
{
    return nullptr != pipeline_layout_.get( );
}

auto PipelineLayout::get( ) const -> vk::PipelineLayout const&
{
    return pipeline_layout_.get( );
}

auto PipelineLayout::get( ) -> vk::PipelineLayout&
{
    return pipeline_layout_.get( );
}

} // namespace ltb::vlk
