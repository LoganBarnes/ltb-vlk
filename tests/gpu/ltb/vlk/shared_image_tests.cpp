// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/vlk/shared_image.hpp"

#if defined( LTB_IPC_SUPPORTED )

// project
#include "ltb/vlk/buffer.hpp"
#include "ltb/vlk/check.hpp"
#include "ltb/vlk/command_buffer.hpp"
#include "ltb/vlk/command_pool.hpp"
#include "ltb/vlk/objs/vulkan_gpu.hpp"

// external
#include <gtest/gtest.h>

// standard
#include <array>
#include <cstdlib>
#include <functional>
#include <string_view>

namespace ltb::vlk
{
namespace
{

constexpr auto image_settings = SharedImageSettings{
    .extent = { 4U, 4U },
    .format = vk::Format::eR8G8B8A8Unorm,
    .usage  = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc,
};

auto color_range( ) -> vk::ImageSubresourceRange
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

    VK_CHECK( commands.begin( vk::CommandBufferBeginInfo{ } ) );
    record( commands );
    VK_CHECK( commands.end( ) );

    auto const queue = gpu.device( ).queues( ).at( QueueType::Graphics );
    VK_CHECK( queue.submit( vk::SubmitInfo{ }.setCommandBuffers( commands ) ) );
    VK_CHECK( queue.waitIdle( ) );
    return utils::success( );
}

/// \brief Two devices stand in for two processes.
class SharedImageTest : public ::testing::Test
{
protected:
    void SetUp( ) override
    {
        auto const settings = objs::VulkanGpuSettings{
            .device = { .extensions = shared_image_device_extensions( ) },
        };
        auto result = exporter_.initialize( settings ).and_then( [ this, &settings ]( ) {
            return importer_.initialize( settings );
        } );
        if ( !result )
        {
            auto const* const require_gpu = std::getenv( "LTB_VLK_REQUIRE_GPU" );
            if ( ( nullptr != require_gpu ) && ( std::string_view{ require_gpu } == "1" ) )
            {
                FAIL( ) << "No device that can share images: " << result.error( ).error_message( );
            }
            GTEST_SKIP( ) << "No device that can share images: "
                          << result.error( ).error_message( );
        }
    }

    objs::VulkanGpu exporter_ = { };
    objs::VulkanGpu importer_ = { };
};

TEST_F( SharedImageTest, CreateFillsInTheInfoAnImporterNeeds )
{
    auto image = SharedImage{ exporter_.device( ) };
    ASSERT_TRUE( image.create( image_settings ).has_value( ) );

    EXPECT_TRUE( image.is_initialized( ) );
    EXPECT_EQ( image.info( ).settings.extent, image_settings.extent );
    EXPECT_EQ( image.info( ).settings.format, image_settings.format );
    EXPECT_GE( image.info( ).allocation_size, image.image( ).memory_requirements( ).size );
}

TEST_F( SharedImageTest, RejectsInvalidSettingsAndHandles )
{
    auto image = SharedImage{ exporter_.device( ) };
    EXPECT_FALSE( image.create( { .format = vk::Format::eR8G8B8A8Unorm } ).has_value( ) );
    EXPECT_FALSE( image.export_handle( ).has_value( ) );

    auto imported = SharedImage{ importer_.device( ) };
    EXPECT_FALSE(
        imported.import( { .settings = image_settings }, ipc::NativeHandle{ } ).has_value( )
    );
    EXPECT_FALSE( imported.is_initialized( ) );
}

TEST_F( SharedImageTest, ImporterSeesTheExportersPixels )
{
    constexpr auto pixel = std::array< uint8, 4 >{ 255U, 0U, 255U, 255U };

    auto exported = SharedImage{ exporter_.device( ) };
    ASSERT_TRUE( exported.create( image_settings ).has_value( ) );

    auto const cleared = submit_and_wait( exporter_, [ & ]( vk::CommandBuffer const& commands ) {
        commands.pipelineBarrier(
            vk::PipelineStageFlagBits::eTopOfPipe,
            vk::PipelineStageFlagBits::eTransfer,
            { },
            { },
            { },
            vk::ImageMemoryBarrier{ }
                .setImage( exported.image( ).get( ) )
                .setSubresourceRange( color_range( ) )
                .setOldLayout( vk::ImageLayout::eUndefined )
                .setNewLayout( vk::ImageLayout::eTransferDstOptimal )
                .setDstAccessMask( vk::AccessFlagBits::eTransferWrite )
                .setSrcQueueFamilyIndex( vk::QueueFamilyIgnored )
                .setDstQueueFamilyIndex( vk::QueueFamilyIgnored )
        );
        commands.clearColorImage(
            exported.image( ).get( ),
            vk::ImageLayout::eTransferDstOptimal,
            vk::ClearColorValue{ std::array{ 1.0F, 0.0F, 1.0F, 1.0F } },
            color_range( )
        );
        // Hand the image to the external queue family.
        commands.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eBottomOfPipe,
            { },
            { },
            { },
            vk::ImageMemoryBarrier{ }
                .setImage( exported.image( ).get( ) )
                .setSubresourceRange( color_range( ) )
                .setOldLayout( vk::ImageLayout::eTransferDstOptimal )
                .setNewLayout( vk::ImageLayout::eTransferSrcOptimal )
                .setSrcAccessMask( vk::AccessFlagBits::eTransferWrite )
                .setSrcQueueFamilyIndex( graphics_family( exporter_ ) )
                .setDstQueueFamilyIndex( vk::QueueFamilyExternal )
        );
    } );
    ASSERT_TRUE( cleared.has_value( ) ) << cleared.error( ).error_message( );

    auto handle = exported.export_handle( );
    ASSERT_TRUE( handle.has_value( ) ) << handle.error( ).error_message( );

    auto       imported = SharedImage{ importer_.device( ) };
    auto const success  = imported.import( exported.info( ), std::move( handle.value( ) ) );
    ASSERT_TRUE( success.has_value( ) ) << success.error( ).error_message( );

    // Read the pixels back through the importing device.
    auto readback = Buffer{ importer_.device( ) };
    ASSERT_TRUE( readback
                     .initialize( {
                         .size  = 4U * 4U * pixel.size( ),
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
        commands.pipelineBarrier(
            vk::PipelineStageFlagBits::eTopOfPipe,
            vk::PipelineStageFlagBits::eTransfer,
            { },
            { },
            { },
            vk::ImageMemoryBarrier{ }
                .setImage( imported.image( ).get( ) )
                .setSubresourceRange( color_range( ) )
                .setOldLayout( vk::ImageLayout::eTransferDstOptimal )
                .setNewLayout( vk::ImageLayout::eTransferSrcOptimal )
                .setDstAccessMask( vk::AccessFlagBits::eTransferRead )
                .setSrcQueueFamilyIndex( vk::QueueFamilyExternal )
                .setDstQueueFamilyIndex( graphics_family( importer_ ) )
        );
        commands.copyImageToBuffer(
            imported.image( ).get( ),
            vk::ImageLayout::eTransferSrcOptimal,
            readback.get( ),
            vk::BufferImageCopy{ }
                .setImageSubresource(
                    vk::ImageSubresourceLayers{ }
                        .setAspectMask( vk::ImageAspectFlagBits::eColor )
                        .setLayerCount( 1U )
                )
                .setImageExtent( { 4U, 4U, 1U } )
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
    for ( auto i = 0U; i < 16U; ++i )
    {
        for ( auto channel = 0U; channel < pixel.size( ); ++channel )
        {
            ASSERT_EQ( mapped.value( )[ ( i * pixel.size( ) ) + channel ], pixel[ channel ] )
                << "pixel " << i << " channel " << channel;
        }
    }
}

} // namespace
} // namespace ltb::vlk

#endif // defined( LTB_IPC_SUPPORTED )
