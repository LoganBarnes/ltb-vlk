// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "app.hpp"

// project
#include "ltb/exec/app_defaults.hpp"
#include "ltb/vlk/buffer_utils.hpp"
#include "ltb/vlk/check.hpp"
#include "ltb/vlk/device_memory_utils.hpp"
#include "ltb/vlk/ltb_vlk_config.hpp"

// external
#include "ltb/window/glfw_context.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <spdlog/spdlog.h>

namespace ltb
{
namespace
{

// This is the highest OpenGL version macOS will support.
constexpr auto opengl_major_version = 4;
constexpr auto opengl_minor_version = 0;

constexpr auto window_size = glm::ivec2{ 800, 600 };

auto to_string( GLubyte const* const str ) -> std::string
{
    if ( nullptr == str )
    {
        return { };
    }

    // Technically, we could reinterpret_cast to a `char const*`, but this is safer.
    auto convert = std::ostringstream{ };

    // The max size limitation prevents infinite loops.
    constexpr auto max_string_size = 512;
    for ( auto i = 0; ( i < max_string_size ) && ( str[ i ] != 0 ); ++i )
    {
        convert << str[ i ];
    }
    return convert.str( );
}

[[maybe_unused]] auto get_string( GLenum const name ) -> std::string
{
    return to_string( glGetString( name ) );
}

} // namespace

RawApp::RawApp( ) = default;

auto RawApp::initialize( ) -> utils::Result< void >
{
    glfw_init_ = ::glfwInit( );
    if ( GLFW_FALSE == glfw_init_ )
    {
        return LTB_MAKE_UNEXPECTED_ERROR( "glfwInit( ) failed" );
    }

    constexpr auto default_samples_per_pixel = 4;
    ::glfwWindowHint( GLFW_SAMPLES, default_samples_per_pixel );
    ::glfwWindowHint( GLFW_VISIBLE, GLFW_TRUE );
    ::glfwWindowHint( GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE );
    ::glfwWindowHint( GLFW_DECORATED, GLFW_TRUE );
    ::glfwWindowHint( GLFW_RESIZABLE, GLFW_TRUE );

    ::glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
    ::glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE );
    ::glfwWindowHint( GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API );
    ::glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, opengl_major_version );
    ::glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, opengl_minor_version );

    window_ = ::glfwCreateWindow( window_size.x, window_size.y, "Window", nullptr, nullptr );
    if ( nullptr == window_ )
    {
        return LTB_MAKE_UNEXPECTED_ERROR( "::glfwCreateWindow() failed" );
    }

    // ::glfwMakeContextCurrent( window_ );
    // ::glfwSwapInterval( 1 );

    // if ( 0 != ::gl3wInit( ) )
    // {
    //     return LTB_MAKE_UNEXPECTED_ERROR( "Failed to load OpenGL via gl3wInit()." );
    // }
    //
    // auto const vendor   = get_string( GL_VENDOR );
    // auto const renderer = get_string( GL_RENDERER );
    // auto const version  = get_string( GL_VERSION );
    //
    // spdlog::info( "OpenGL Device: {} {}", vendor, renderer );
    // spdlog::info( "OpenGL Version: {}", version );
    //
    // glClearColor( 0.0f, 0.0f, 1.0f, 0.0f );

    return utils::success( );
}

auto RawApp::running( ) const -> bool
{
    return ( nullptr != window_ ) && !glfwWindowShouldClose( window_ );
}

auto RawApp::render( ) const -> void
{
    ::glfwWaitEvents( );
    // glViewport( 0, 0, window_size.x, window_size.y );
    // glClear( GL_COLOR_BUFFER_BIT );
}

auto RawApp::cleanup( ) -> void
{
    if ( nullptr != window_ )
    {
        ::glfwDestroyWindow( window_ );
        spdlog::debug( "::glfwDestroyWindow( )" );
    }
    if ( GLFW_TRUE == glfw_init_ )
    {
        ::glfwTerminate( );
    }
}

} // namespace ltb
