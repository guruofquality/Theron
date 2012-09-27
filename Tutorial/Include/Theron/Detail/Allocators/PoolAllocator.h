// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_DETAIL_ALLOCATORS_POOLALLOCATOR_H
#define THERON_DETAIL_ALLOCATORS_POOLALLOCATOR_H


#include <Theron/Assert.h>
#include <Theron/BasicTypes.h>
#include <Theron/Defines.h>
#include <Theron/IAllocator.h>

#include <Theron/Detail/Containers/List.h>
#include <Theron/Detail/Allocators/TrivialAllocator.h>


namespace Theron
{
namespace Detail
{


/**
Pool allocator for cached allocation of fixed-size memory blocks.
*/
template <class ItemType, uint32_t ITEM_ALIGNMENT>
class PoolAllocator : public Theron::IAllocator
{
public:

    /**
    Default constructor.
    Constructs a PoolAllocator around an internally owned Detail::TrivialAllocator.
    The TrivialAllocator acts as a trivial wrapper around global new and delete. The
    PoolAllocator adds caching of fixed-sized memory blocks.
    */
    inline PoolAllocator();

    /**
    Explicit constructor.
    Constructs a PoolAllocator around an externally owned lower-level allocator.
    The PoolAllocator adds caching of fixed-sized memory blocks.
    \param allocator Pointer to a lower-level allocator which the cache will wrap.
    */
    inline explicit PoolAllocator(IAllocator *const allocator);

    /**
    Destructor.
    */
    inline virtual ~PoolAllocator();

    /**
    Allocates a new node, reusing a free one if possible.
    \note The size parameter is ignored.
    */
    inline virtual void *Allocate(const uint32_t size);

    /**
    Allocates a new aligned node, reusing a free one if possible.
    \note The size and alignment parameters are ignored.
    */
    inline virtual void *AllocateAligned(const uint32_t size, const uint32_t alignment);

    /**
    Frees a previously allocated node, allowing it to be reused.
    */
    inline virtual void Free(void *const node);

    /**
    Frees a previously allocated node of a known size, allowing it to be reused.
    \note The size parameter is ignored.
    */
    inline virtual void Free(void *const node, const uint32_t size);

private:

    struct FreeNode : public List<FreeNode>::Node
    {
    };

    TrivialAllocator mTrivialAllocator;     ///< Default low-level allocator implementation.
    IAllocator *const mAllocator;           ///< Pointer to a wrapped low-level allocator.
    List<FreeNode> mFreeList;               ///< List of free nodes.
};


template <class ItemType, uint32_t ITEM_ALIGNMENT>
inline PoolAllocator<ItemType, ITEM_ALIGNMENT>::PoolAllocator() :
  mTrivialAllocator(),
  mAllocator(&mTrivialAllocator),
  mFreeList()
{
}


template <class ItemType, uint32_t ITEM_ALIGNMENT>
inline PoolAllocator<ItemType, ITEM_ALIGNMENT>::PoolAllocator(IAllocator *const allocator) :
  mTrivialAllocator(),
  mAllocator(allocator),
  mFreeList()
{
}


template <class ItemType, uint32_t ITEM_ALIGNMENT>
inline PoolAllocator<ItemType, ITEM_ALIGNMENT>::~PoolAllocator()
{
    IAllocator *const allocator(AllocatorManager::Instance().GetAllocator());

    // Free the allocated node structures.
    while (!mFreeList.Empty())
    {
        FreeNode *const freeNode(mFreeList.Front());
        mFreeList.Remove(freeNode);

        // Call the node destructor explicitly since we constructed via in-place new, so can't use delete.
        allocator->Free(freeNode, sizeof(ItemType));
    }
}


template <class ItemType, uint32_t ITEM_ALIGNMENT>
inline void *PoolAllocator<ItemType, ITEM_ALIGNMENT>::Allocate(const uint32_t /*size*/)
{
    // Grab a free node off the free list if one is available.
    if (!mFreeList.Empty())
    {
        FreeNode *const freeNode(mFreeList.Front());
        mFreeList.Remove(freeNode);
        return reinterpret_cast<void *>(freeNode);
    }

    // Allocate a node.
    IAllocator *const allocator(AllocatorManager::Instance().GetAllocator());
    return allocator->AllocateAligned(sizeof(ItemType), ITEM_ALIGNMENT);
}


template <class ItemType, uint32_t ITEM_ALIGNMENT>
inline void *PoolAllocator<ItemType, ITEM_ALIGNMENT>::AllocateAligned(const uint32_t size, const uint32_t /*alignment*/)
{
    return Allocate(size);
}


template <class ItemType, uint32_t ITEM_ALIGNMENT>
inline void PoolAllocator<ItemType, ITEM_ALIGNMENT>::Free(void *const node)
{
    mFreeList.Insert(reinterpret_cast<FreeNode *>(node));
}


template <class ItemType, uint32_t ITEM_ALIGNMENT>
inline void PoolAllocator<ItemType, ITEM_ALIGNMENT>::Free(void *const node, const uint32_t /*size*/)
{
    return Free(node);
}


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_ALLOCATORS_POOLALLOCATOR_H
