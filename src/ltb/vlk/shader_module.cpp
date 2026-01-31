// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/vlk/shader_module.hpp"

// project
#include "ltb/utils/file_utils.hpp"
#include "ltb/utils/types.hpp"
#include "ltb/vlk/check.hpp"
#include "ltb/vlk/device.hpp"

// external
#include <spdlog/spdlog.h>

namespace ltb::vlk
{

ShaderModule::ShaderModule( Device& device )
    : device_( device )
{
}

auto ShaderModule::initialize( ShaderModuleSettings settings ) -> utils::Result< void >
{
    if ( this->is_initialized( ) )
    {
        return utils::success( );
    }
    LTB_CHECK_VALID( device_.is_initialized( ) );

    LTB_CHECK( auto const code, utils::get_binary_file_contents< uint32 >( settings.spirv_file ) );

    auto const create_info = vk::ShaderModuleCreateInfo{ }.setCode( code );
    VK_CHECK( auto shader_module, device_.get( ).createShaderModuleUnique( create_info ) );
    spdlog::debug( "vk::createShaderModuleUnique() for '{}'", settings.spirv_file.string( ) );

    settings_      = std::move( settings );
    shader_module_ = std::move( shader_module );

    return utils::success( );
}

auto ShaderModule::is_initialized( ) const -> bool
{
    return nullptr != shader_module_.get( );
}

auto ShaderModule::get( ) const -> vk::ShaderModule const&
{
    return shader_module_.get( );
}

auto ShaderModule::get( ) -> vk::ShaderModule&
{
    return shader_module_.get( );
}

auto ShaderModule::settings( ) const -> ShaderModuleSettings const&
{
    return settings_;
}

auto ShaderModule::get_shader_stage_create_info( ) const -> vk::PipelineShaderStageCreateInfo
{
    return vk::PipelineShaderStageCreateInfo{ }
        .setStage( settings_.stage )
        .setModule( this->get( ) )
        .setPName( settings_.name );
}

auto GetShaderStageCreateInfo::operator( )( ShaderModule const& shader_module ) const
    -> vk::PipelineShaderStageCreateInfo
{
    return shader_module.get_shader_stage_create_info( );
}

} // namespace ltb::vlk
