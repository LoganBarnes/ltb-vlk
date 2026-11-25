// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/ipc/native_handle.hpp"

// standard
#include <cstddef>
#include <vector>

namespace ltb::ipc
{

/// \brief The most native handles a single message can carry.
inline constexpr auto max_handles_per_message = 16UZ;

#if defined( __linux__ )

/// \brief What a non-blocking receive found.
struct ReceivedMessage
{
    /// \brief The number of bytes written into the receive buffer.
    std::size_t size = 0U;

    /// \brief Handles sent along with the message, now owned by the receiver.
    std::vector< NativeHandle > handles = { };
};

#endif

enum class ReceiveStatus
{
    Message, ///< A message was received.
    WouldBlock, ///< Nothing to receive right now.
    Closed, ///< The other end closed the connection.
};

enum class SendStatus
{
    Sent, ///< The message was sent.
    Closed, ///< The other end closed the connection, so nothing was sent.
};

#if defined( __linux__ )

struct ReceiveResult
{
    ReceiveStatus   status  = ReceiveStatus::WouldBlock;
    ReceivedMessage message = { };
};

#endif

} // namespace ltb::ipc
