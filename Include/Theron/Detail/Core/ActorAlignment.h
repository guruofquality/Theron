// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_DETAIL_CORE_ACTORALIGNMENT_H
#define THERON_DETAIL_CORE_ACTORALIGNMENT_H


#include <Theron/BasicTypes.h>


namespace Theron
{
namespace Detail
{


/// \brief Traits struct template that stores alignment information about actors.
/// Users can specialize this template for their own actor types in order to tell
/// Theron about any specialized alignment requirements of those classes.
/// Theron uses the alignment value defined for each actor type to request memory
/// with the correct alignment from the general allocator registered with the
/// \ref AllocatorManager. The default alignment is four bytes, implying that
/// instances of the actor class should be allocated starting at 4-byte boundaries.
/// \note Note that although Theron will request memory allocated with the correct
/// alignment, whether or not the allocator respects the alignment request is up
/// to the allocator implementation. The default allocator, DefaultAllocator,
/// supports alignment and returns correctly aligned allocations. Users with
/// specialized alignment requirements should ensure that any custom allocator,
/// implementing the \ref IAllocator interface, and enabled with
/// \ref Theron::AllocatorManager::SetAllocator, supports aligned allocations.
/// \tparam ActorType The actor type for which the alignment traits are defined.
template <class ActorType>
struct ActorAlignment
{
    /// \brief Describes the memory alignment requirement of the actor type, in bytes.
    /// The default alignment is four bytes.
    static const uint32_t ALIGNMENT = 4;
};


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_CORE_ACTORALIGNMENT_H

