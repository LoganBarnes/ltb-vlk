// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#if defined( __linux__ )

// project
#include "ltb/vlk/buffer.hpp"
#include "ltb/vlk/check.hpp"
#include "ltb/vlk/command_buffer.hpp"
#include "ltb/vlk/command_pool.hpp"
#include "ltb/vlk/device_memory.hpp"
#include "ltb/vlk/image.hpp"
#include "ltb/vlk/objs/vulkan_gpu.hpp"

// external
#include <gtest/gtest.h>

// system
#include <unistd.h>

// standard
#include <array>
#include <cstdlib>
#include <functional>
#include <string_view>

namespace ltb::vlk
{
namespace
{

constexpr auto image_extent = vk::Extent3D{ 4U, 4U, 1U };
constexpr auto image_format = vk::Format::eR8G8B8A8Unorm;

// Images that share memory must be created with identical settings.
auto shared_image_settings( ) -> ImageSettings
{
    return {
        .extent = image_extent,
        .format = image_format,
        .usage  = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc,
        .external_memory_handle_types = vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd,
    };
}

auto color_subresource_range( ) -> vk::ImageSubresourceRange
{
    return vk::ImageSubresourceRange{ }
        .setAspectMask( vk::ImageAspectFlagBits::eColor )
        .setLevelCount( 1U )
        .setLayerCount( 1U );
}

auto graphics_family( objs::VulkanGpu const& gpu ) -> uint32
{
    return gpu.physical_device( ).queue_families( ).at( QueueType::Graphics );
}

/// \brief Records commands into a one-time command buffer, submits them, and waits.
auto submit_and_wait(
    objs::VulkanGpu&                                  gpu,
    std::function< void( vk::CommandBuffer const& ) > record
) -> utils::Result< void >
{
    auto pool = CommandPool{ gpu.device( ), gpu.physical_device( ) };
    LTB_CHECK( pool.initialize( ) );

    auto command_buffer = CommandBuffer{ gpu.device( ), pool };
    LTB_CHECK( command_buffer.initialize( ) );
    auto const& commands = command_buffer.get( );

    VK_CHECK( commands.begin(
        vk::CommandBufferBeginInfo{ }.setFlags( vk::CommandBufferUsageFlagBits::eOneTimeSubmit )
    ) );
    record( commands );
    VK_CHECK( commands.end( ) );

    auto const queue = gpu.device( ).queues( ).at( QueueType::Graphics );
    VK_CHECK( queue.submit( vk::SubmitInfo{ }.setCommandBuffers( commands ) ) );
    VK_CHECK( queue.waitIdle( ) );

    return utils::success( );
}

class ExternalMemoryTest : public ::testing::Test
{
protected:
    void SetUp( ) override
    {
        auto const settings = objs::VulkanGpuSettings{
            .device = { .extensions = { VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME } },
        };

        auto result = exporter_.initialize( settings ).and_then( [ this, &settings ] {
            return importer_.initialize( settings );
        } );
        if ( !result )
        {
            auto const* const require_gpu = std::getenv( "LTB_VLK_REQUIRE_GPU" );
            if ( ( nullptr != require_gpu ) && ( std::string_view{ require_gpu } == "1" ) )
            {
                FAIL( ) << "No device with VK_KHR_external_memory_fd: "
                        << result.error( ).error_message( );
            }
            GTEST_SKIP( ) << "No device with VK_KHR_external_memory_fd: "
                          << result.error( ).error_message( );
        }
    }

    // Two separate devices stand in for two processes.
    objs::VulkanGpu exporter_ = { };
    objs::VulkanGpu importer_ = { };
};

TEST_F( ExternalMemoryTest, BothDevicesHaveTheSameIdentity )
{
    EXPECT_EQ( exporter_.physical_device( ).identity( ), importer_.physical_device( ).identity( ) );
}

TEST_F( ExternalMemoryTest, ExportFdRequiresAnExportableAllocation )
{
    auto memory = DeviceMemory{ exporter_.device( ) };
    ASSERT_TRUE( memory.initialize( { .allocation_size = 256U } ).has_value( ) );

    EXPECT_FALSE( memory.export_fd( ).has_value( ) );
}

TEST_F( ExternalMemoryTest, ImportedImageSeesExportedPixels )
{
    constexpr auto clear_color = std::array{ 0.0F, 1.0F, 0.0F, 1.0F };
    constexpr auto pixel_bytes = std::array< uint8, 4 >{ 0U, 255U, 0U, 255U };

    // ----- Exporter: clear an image and export its memory -----
    auto exported_image = Image{ exporter_.device( ) };
    ASSERT_TRUE( exported_image.initialize( shared_image_settings( ) ).has_value( ) );

    auto const requirements = exported_image.memory_requirements( );
    auto const memory_type
        = exporter_.physical_device( )
              .find_memory_type_index( { requirements }, vk::MemoryPropertyFlagBits::eDeviceLocal );
    ASSERT_TRUE( memory_type.has_value( ) );

    auto exported_memory = DeviceMemory{ exporter_.device( ) };
    ASSERT_TRUE( exported_memory
                     .initialize( {
                         .allocation_size     = requirements.size,
                         .memory_type_index   = memory_type.value( ),
                         .export_handle_types = vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd,
                         .dedicated_image     = exported_image.get( ),
                     } )
                     .has_value( ) );
    ASSERT_EQ(
        exporter_.device( )
            .get( )
            .bindImageMemory( exported_image.get( ), exported_memory.get( ), 0U ),
        vk::Result::eSuccess
    );

    auto const cleared = submit_and_wait( exporter_, [ & ]( vk::CommandBuffer const& commands ) {
        commands.pipelineBarrier(
            vk::PipelineStageFlagBits::eTopOfPipe,
            vk::PipelineStageFlagBits::eTransfer,
            { },
            { },
            { },
            vk::ImageMemoryBarrier{ }
                .setImage( exported_image.get( ) )
                .setSubresourceRange( color_subresource_range( ) )
                .setOldLayout( vk::ImageLayout::eUndefined )
                .setNewLayout( vk::ImageLayout::eTransferDstOptimal )
                .setDstAccessMask( vk::AccessFlagBits::eTransferWrite )
                .setSrcQueueFamilyIndex( vk::QueueFamilyIgnored )
                .setDstQueueFamilyIndex( vk::QueueFamilyIgnored )
        );
        commands.clearColorImage(
            exported_image.get( ),
            vk::ImageLayout::eTransferDstOptimal,
            vk::ClearColorValue{ clear_color },
            color_subresource_range( )
        );
        // Release ownership to the "external" queue family so another device can use it.
        commands.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eBottomOfPipe,
            { },
            { },
            { },
            vk::ImageMemoryBarrier{ }
                .setImage( exported_image.get( ) )
                .setSubresourceRange( color_subresource_range( ) )
                .setOldLayout( vk::ImageLayout::eTransferDstOptimal )
                .setNewLayout( vk::ImageLayout::eTransferSrcOptimal )
                .setSrcAccessMask( vk::AccessFlagBits::eTransferWrite )
                .setSrcQueueFamilyIndex( graphics_family( exporter_ ) )
                .setDstQueueFamilyIndex( vk::QueueFamilyExternal )
        );
    } );
    ASSERT_TRUE( cleared.has_value( ) ) << cleared.error( ).error_message( );

    auto exported_fd = exported_memory.export_fd( );
    ASSERT_TRUE( exported_fd.has_value( ) ) << exported_fd.error( ).error_message( );

    // ----- Importer: import the memory and copy the pixels back to the host -----
    auto imported_image = Image{ importer_.device( ) };
    ASSERT_TRUE( imported_image.initialize( shared_image_settings( ) ).has_value( ) );
    ASSERT_TRUE(
        ( imported_image.memory_requirements( ).memoryTypeBits >> memory_type.value( ) ) & 1U
    );

    auto       imported_memory = DeviceMemory{ importer_.device( ) };
    auto const imported        = imported_memory.initialize( {
        .allocation_size   = requirements.size,
        .memory_type_index = memory_type.value( ),
        .import_fd         = exported_fd.value( ),
        .dedicated_image   = imported_image.get( ),
    } );
    if ( !imported )
    {
        ::close( exported_fd.value( ) ); // Still ours if the import failed.
    }
    ASSERT_TRUE( imported.has_value( ) ) << imported.error( ).error_message( );
    ASSERT_EQ(
        importer_.device( )
            .get( )
            .bindImageMemory( imported_image.get( ), imported_memory.get( ), 0U ),
        vk::Result::eSuccess
    );

    auto readback = Buffer{ importer_.device( ) };
    ASSERT_TRUE( readback
                     .initialize( {
                         .size  = image_extent.width * image_extent.height * pixel_bytes.size( ),
                         .usage = vk::BufferUsageFlagBits::eTransferDst,
                     } )
                     .has_value( ) );
    auto const readback_type = importer_.physical_device( ).find_memory_type_index(
        { readback.memory_requirements( ) },
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );
    ASSERT_TRUE( readback_type.has_value( ) );
    auto readback_memory = DeviceMemory{ importer_.device( ) };
    ASSERT_TRUE( readback_memory
                     .initialize( {
                         .allocation_size   = readback.memory_requirements( ).size,
                         .memory_type_index = readback_type.value( ),
                     } )
                     .has_value( ) );
    ASSERT_EQ(
        importer_.device( ).get( ).bindBufferMemory( readback.get( ), readback_memory.get( ), 0U ),
        vk::Result::eSuccess
    );

    auto const copied = submit_and_wait( importer_, [ & ]( vk::CommandBuffer const& commands ) {
        // Acquire ownership from the external queue family (matching the release above).
        commands.pipelineBarrier(
            vk::PipelineStageFlagBits::eTopOfPipe,
            vk::PipelineStageFlagBits::eTransfer,
            { },
            { },
            { },
            vk::ImageMemoryBarrier{ }
                .setImage( imported_image.get( ) )
                .setSubresourceRange( color_subresource_range( ) )
                .setOldLayout( vk::ImageLayout::eTransferDstOptimal )
                .setNewLayout( vk::ImageLayout::eTransferSrcOptimal )
                .setDstAccessMask( vk::AccessFlagBits::eTransferRead )
                .setSrcQueueFamilyIndex( vk::QueueFamilyExternal )
                .setDstQueueFamilyIndex( graphics_family( importer_ ) )
        );
        commands.copyImageToBuffer(
            imported_image.get( ),
            vk::ImageLayout::eTransferSrcOptimal,
            readback.get( ),
            vk::BufferImageCopy{ }
                .setImageSubresource(
                    vk::ImageSubresourceLayers{ }
                        .setAspectMask( vk::ImageAspectFlagBits::eColor )
                        .setLayerCount( 1U )
                )
                .setImageExtent( image_extent )
        );
        commands.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eHost,
            { },
            vk::MemoryBarrier{ }
                .setSrcAccessMask( vk::AccessFlagBits::eTransferWrite )
                .setDstAccessMask( vk::AccessFlagBits::eHostRead ),
            { },
            { }
        );
    } );
    ASSERT_TRUE( copied.has_value( ) ) << copied.error( ).error_message( );

    auto const mapped = readback_memory.map( );
    ASSERT_TRUE( mapped.has_value( ) );
    auto const* const pixels = mapped.value( );

    auto const pixel_count = image_extent.width * image_extent.height;
    for ( auto i = 0U; i < pixel_count; ++i )
    {
        for ( auto channel = 0U; channel < pixel_bytes.size( ); ++channel )
        {
            ASSERT_EQ( pixels[ ( i * pixel_bytes.size( ) ) + channel ], pixel_bytes[ channel ] )
                << "pixel " << i << " channel " << channel;
        }
    }
}

} // namespace
} // namespace ltb::vlk

#endif // defined( __linux__ )
