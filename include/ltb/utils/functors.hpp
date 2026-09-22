// ///////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Logan Barnes - All Rights Reserved
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

namespace ltb::utils
{

template < typename Type >
struct Scale
{
    Type scale = Type( 1 );

    auto operator( )( Type const& value ) const -> Type
    {
        return scale * value;
    }
};

} // namespace ltb::utils
