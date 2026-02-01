// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "app.hpp"

namespace ltb
{

HeadlessApp::HeadlessApp( ) = default;

auto HeadlessApp::initialize( ) -> utils::Result< exec::UpdateLoopStatus >
{
    return exec::UpdateLoopStatus{ };
}

auto HeadlessApp::is_initialized( ) const -> bool
{
    return true;
}

auto HeadlessApp::fixed_step_update( exec::UpdateLoopStatus const& status ) -> exec::UpdateRequests
{
    auto requests = status.requests;

    // Only a single iteration of this app will run.
    requests.exit_update_loop = true;

    return requests;
}

auto HeadlessApp::frame_update( exec::UpdateLoopStatus const& status ) -> exec::UpdateRequests
{
    return status.requests;
}

auto HeadlessApp::on_resize( glm::ivec2 size ) -> utils::Result< void >
{
    utils::ignore( size );
    return utils::success( );
}

} // namespace ltb
