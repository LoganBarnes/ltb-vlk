// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/utils/types.hpp"
#include "ltb/vlk/image.hpp"
#include "ltb/vlk/physical_device.hpp"

// standard
#include <array>
#include <cstring>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>

/// Shared by the producer (`--share`) and display (`--display`) processes.
///
/// Protocol, over a SOCK_SEQPACKET Unix socket (one struct per message):
///
///   producer -> display  HelloMessage + one memory fd per image slot (SCM_RIGHTS)
///   producer -> display  FrameMessage{ FrameReady, slot }    The slot holds a finished frame.
///   display  -> producer FrameMessage{ SlotReleased, slot }  The display is done reading it.
///   display  -> producer RejectedMessage                     Why it's being disconnected.
///
/// A producer only renders into slots the display has released, so the display can never
/// read a half-written frame. Ownership of each image is handed over with queue family
/// ownership transfers to and from VK_QUEUE_FAMILY_EXTERNAL.
namespace ltb::share
{

/// \brief Bumped whenever a message struct changes.
inline constexpr auto protocol_version = 1U;

/// \brief Each producer double-buffers its output.
inline constexpr auto image_slot_count = 2U;

/// \brief The display draws at most this many producers.
inline constexpr auto max_producers = 16U;

inline constexpr auto shared_image_format = vk::Format::eR8G8B8A8Unorm;

/// \brief The layout a producer leaves an image in when it hands it to the display.
inline constexpr auto shared_image_layout = vk::ImageLayout::eColorAttachmentOptimal;

/// \brief Images that share memory must be created with identical settings in both processes.
auto shared_image_settings( vk::Extent2D extent ) -> vlk::ImageSettings;

enum class MessageType : uint32
{
    Hello        = 1U,
    FrameReady   = 2U,
    SlotReleased = 3U,
    Rejected     = 4U,
};

struct HelloMessage
{
    MessageType type    = MessageType::Hello;
    uint32      version = protocol_version;

    uint32 width      = 0U;
    uint32 height     = 0U;
    uint32 slot_count = image_slot_count;

    // Needed to import the memory. Every slot uses the same values.
    uint32 memory_type_index = 0U;
    uint64 allocation_size   = 0U;

    // The memory can only be imported on the same GPU and driver.
    vlk::DeviceIdentity device = { };

    int32 process_id = 0;
};

struct FrameMessage
{
    MessageType type         = MessageType::FrameReady;
    uint32      slot         = 0U;
    uint64      frame_number = 0U;
};

/// \brief Sent by the display just before it disconnects a producer it can't use.
struct RejectedMessage
{
    MessageType             type   = MessageType::Rejected;
    std::array< char, 256 > reason = { }; ///< Null-terminated, possibly truncated.
};

auto make_rejected_message( std::string_view reason ) -> RejectedMessage;

/// \brief The reason in a rejected message as a string.
auto rejection_reason( RejectedMessage const& message ) -> std::string;

/// \brief Copies a received message into `T` if it's the right size and type.
template < typename T >
auto read_message( std::span< std::byte const > bytes, MessageType expected_type )
    -> std::optional< T >
{
    if ( bytes.size( ) != sizeof( T ) )
    {
        return std::nullopt;
    }
    auto message = T{ };
    std::memcpy( &message, bytes.data( ), sizeof( T ) );
    if ( message.type != expected_type )
    {
        return std::nullopt;
    }
    return message;
}

/// \brief The type of a received message, or `std::nullopt` if it's too short to have one.
auto peek_message_type( std::span< std::byte const > bytes ) -> std::optional< MessageType >;

/// \brief `$XDG_RUNTIME_DIR/ltb-vlk-share.sock`, or a per-user path in /tmp without it.
auto default_socket_path( ) -> std::filesystem::path;

/// \brief Makes Ctrl+C (SIGINT) and SIGTERM request a clean shutdown instead of killing
///        the process, so Vulkan objects and the socket file are cleaned up.
auto install_stop_signal_handlers( ) -> void;

/// \brief True once SIGINT or SIGTERM has been received.
auto stop_requested( ) -> bool;

} // namespace ltb::share
