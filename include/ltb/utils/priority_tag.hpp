// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

namespace ltb::utils
{

/// \brief Allows SFINAE functions to be called in a certain order:
///
/// \code
/// auto foo(int bar, priority_tag<2>) -> decltype(<code>); // called first if available
/// auto foo(int bar, priority_tag<1>) -> decltype(<code>);
/// auto foo(int bar, priority_tag<0>) -> decltype(<code>); // called last
/// \endcode
template < unsigned N >
struct PriorityTag : public PriorityTag< N - 1 >
{
};

template <>
struct PriorityTag< 0u >
{
};

} // namespace ltb::utils
