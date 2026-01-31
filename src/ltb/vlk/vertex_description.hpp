// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/utils/types.hpp"
#include "ltb/vlk/vulkan.hpp"

namespace ltb::vlk
{

struct VertexInputBindingDescriptions
{
    template < typename VertexType >
    auto add( ) -> VertexInputBindingDescriptions&;

    std::vector< vk::VertexInputBindingDescription > descriptions = { };
};

template < typename VertexType >
auto VertexInputBindingDescriptions::add( ) -> VertexInputBindingDescriptions&
{
    auto const binding = static_cast< uint32 >( descriptions.size( ) );
    descriptions.emplace_back(
        vk::VertexInputBindingDescription{ }
            .setBinding( binding )
            .setStride( static_cast< uint32 >( sizeof( VertexType ) ) )
            .setInputRate( vk::VertexInputRate::eVertex )
    );
    return *this;
}

struct VertexInputAttributeDescriptions
{
    template < typename T >
    auto add( vk::Format format, uint32 offset = 0 ) -> VertexInputAttributeDescriptions&;

    std::vector< vk::VertexInputAttributeDescription > descriptions = { };
};

template < typename T >
auto VertexInputAttributeDescriptions::add( vk::Format format, uint32 offset )
    -> VertexInputAttributeDescriptions&
{
    auto const binding_and_location = static_cast< uint32 >( descriptions.size( ) );
    descriptions.emplace_back(
        vk::VertexInputAttributeDescription{ }
            .setBinding( binding_and_location )
            .setLocation( binding_and_location )
            .setFormat( format )
            .setOffset( offset )
    );
    return *this;
}

class VertexInputDescriptions
{
public:
    template < typename T >
    auto add( vk::Format format, uint32 offset = 0 ) -> VertexInputDescriptions&;

    [[nodiscard]] auto bindings( ) const -> std::vector< vk::VertexInputBindingDescription > const&;
    [[nodiscard]] auto attributes( ) const
        -> std::vector< vk::VertexInputAttributeDescription > const&;

private:
    VertexInputBindingDescriptions   bindings_   = { };
    VertexInputAttributeDescriptions attributes_ = { };
};

template < typename T >
auto VertexInputDescriptions::add( vk::Format const format, uint32 const offset )
    -> VertexInputDescriptions&
{
    bindings_.add< T >( );
    attributes_.add< T >( format, offset );
    return *this;
}

} // namespace ltb::vlk
