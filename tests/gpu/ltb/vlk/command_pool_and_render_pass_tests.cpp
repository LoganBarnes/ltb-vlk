// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/vlk/command_pool.hpp"
#include "ltb/vlk/render_pass.hpp"

// project
#include "ltb/vlk/headless_gpu_fixture.hpp"

namespace ltb::vlk
{
namespace
{

using CommandPoolTest = HeadlessGpuTest;
using RenderPassTest  = HeadlessGpuTest;

TEST_F( CommandPoolTest, DefaultSettingsUseTheGraphicsQueue )
{
    auto pool = CommandPool{ gpu_.device( ), gpu_.physical_device( ) };

    auto const result = pool.initialize( );
    ASSERT_TRUE( result.has_value( ) ) << result.error( ).error_message( );
    EXPECT_TRUE( pool.is_initialized( ) );
}

TEST_F( RenderPassTest, StoredSettingsPointAtTheirOwnAttachmentReferences )
{
    auto render_pass = RenderPass{ gpu_.device( ) };

    auto const result = render_pass.initialize(
        render_pass_settings_3d( vk::Format::eB8G8R8A8Unorm, vk::Format::eD32Sfloat )
    );
    ASSERT_TRUE( result.has_value( ) ) << result.error( ).error_message( );

    auto const& settings = render_pass.settings( );
    ASSERT_EQ( settings.subpasses.size( ), 1U );
    EXPECT_EQ( settings.subpasses[ 0 ].pDepthStencilAttachment, &settings.depth_attachment_ref );
    EXPECT_EQ( settings.subpasses[ 0 ].pColorAttachments, settings.color_attachment_refs.data( ) );
}

} // namespace
} // namespace ltb::vlk
