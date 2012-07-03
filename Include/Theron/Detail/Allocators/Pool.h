// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_DETAIL_ALLOCATORS_POOL_H
#define THERON_DETAIL_ALLOCATORS_POOL_H


#include <Theron/Align.h>
#include <Theron/BasicTypes.h>
#include <Theron/Defines.h>

#include <Theron/Detail/Debug/Assert.h>


namespace Theron
{
namespace Detail
{


/// A list of free memory blocks.
class Pool
{
public:

    /// Constructor.
    inline Pool();

    /// Returns true if the pool contains no memory blocks.
    inline bool Empty() const;

    /// Adds a memory block to the pool.
    inline bool Add(void *memory);

    /// Retreives a memory block from the pool with the given alignment.
    /// \return Zero if no suitable blocks in pool.
    inline void *FetchAligned(const uint32_t alignment);

    /// Retreives a memory block from the pool with any alignment.
    /// \return Zero if no blocks in pool.
    inline void *Fetch();

private:

    /// A node representing a free memory block within the pool.
    /// Nodes are created in-place within the free blocks they represent.
    struct Node
    {
        THERON_FORCEINLINE Node() : mNext(0)
        {
        }

        Node *mNext;                        ///< Pointer to next node in a list.
    };

    static const uint32_t MAX_BLOCKS = 16;  ///< Maximum number of memory blocks stored per pool.

    Node mHead;                             ///< Dummy node at head of a linked list of nodes in the pool.
    uint32_t mBlockCount;                   ///< Number of blocks currently cached in the pool.
};


THERON_FORCEINLINE Pool::Pool() :
  mHead(),
  mBlockCount(0)
{
}


THERON_FORCEINLINE bool Pool::Empty() const
{
    THERON_ASSERT((mBlockCount == 0 && mHead.mNext == 0) || (mBlockCount != 0 && mHead.mNext != 0));
    return (mBlockCount == 0);
}

    
THERON_FORCEINLINE bool Pool::Add(void *const memory)
{
    THERON_ASSERT(memory);

    // Just call it a node and link it in.
    Node *const node(reinterpret_cast<Node *>(memory));

    // Below maximum block count limit?
    if (mBlockCount < MAX_BLOCKS)
    {
        node->mNext = mHead.mNext;
        mHead.mNext = node;
        ++mBlockCount;
        return true;
    }

    return false;
}


THERON_FORCEINLINE void *Pool::FetchAligned(const uint32_t alignment)
{
    Node *previous(&mHead);
    const uint32_t alignmentMask(alignment - 1);

    // Search the block list.
    Node *node(mHead.mNext);
    while (node)
    {
        // Prefetch.
        Node *const next(node->mNext);

        // This is THERON_ALIGNED with the alignment mask calculated outside the loop.
        if ((reinterpret_cast<uintptr_t>(node) & alignmentMask) == 0)
        {
            // Remove from list and return as block.
            previous->mNext = next;
            --mBlockCount;
            return reinterpret_cast<void *>(node);
        }

        previous = node;
        node = next;
    }

    // Zero result indicates no correctly aligned block available.
    return 0;
}


THERON_FORCEINLINE void *Pool::Fetch()
{
    // Grab first block in the list if the list isn't empty.
    Node *const node(mHead.mNext);
    if (node)
    {
        mHead.mNext = node->mNext;
        --mBlockCount;
        return reinterpret_cast<void *>(node);
    }

    // Zero result indicates no correctly aligned block available.
    return 0;
}


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_ALLOCATORS_POOL_H

