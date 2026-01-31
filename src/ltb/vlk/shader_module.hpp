// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/utils/result.hpp"
#include "ltb/vlk/fwd.hpp"
#include "ltb/vlk/vulkan.hpp"

// standard
#include <filesystem>

namespace ltb::vlk
{

struct ShaderModuleSettings
{
    std::filesystem::path   spirv_file = { };
    vk::ShaderStageFlagBits stage      = { };
    char const*             name       = "main";
};

class ShaderModule
{
public:
    explicit( false ) ShaderModule( Device& device );

    auto initialize( ShaderModuleSettings settings ) -> utils::Result< void >;

    [[nodiscard( "Const getter" )]]
    auto is_initialized( ) const -> bool;

    [[nodiscard( "Const getter" )]]
    auto get( ) const -> vk::ShaderModule const&;
    auto get( ) -> vk::ShaderModule&;

    [[nodiscard( "Const getter" )]]
    auto settings( ) const -> ShaderModuleSettings const&;

    [[nodiscard( "Const getter" )]]
    auto get_shader_stage_create_info( ) const -> vk::PipelineShaderStageCreateInfo;

private:
    Device& device_;

    ShaderModuleSettings   settings_      = { };
    vk::UniqueShaderModule shader_module_ = { };
};

struct GetShaderStageCreateInfo
{
    auto operator( )( ShaderModule const& shader_module ) const
        -> vk::PipelineShaderStageCreateInfo;
};

} // namespace ltb::vlk
