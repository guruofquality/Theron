// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_DETAIL_PAGEDPOOL_FREELIST_H
#define THERON_DETAIL_PAGEDPOOL_FREELIST_H


#include <Theron/BasicTypes.h>
#include <Theron/Defines.h>

#include <Theron/Detail/Debug/Assert.h>


namespace Theron
{
namespace Detail
{


class FreeList
{
public:

    inline FreeList() : mHead(0), mCount(0)
    {
    }

    /// Returns the number of entries currently on this free list.
    THERON_FORCEINLINE uint32_t Count() const
    {
        return mCount;
    }

    /// Empties the free list, forgetting all of its entries.
    THERON_FORCEINLINE void Clear()
    {
        mHead = 0;
        mCount = 0;
    }

    /// Allocates an entry from the free list, if it contains any.
    THERON_FORCEINLINE void *Get()
    {
        if (mHead)
        {
            THERON_ASSERT(mCount > 0);

            FreeNode *const node(mHead);
            mHead = mHead->mNext;
            --mCount;

            return node;
        }

        return 0;
    }

    /// Returns an entry to the free list.
    THERON_FORCEINLINE void Add(void *const entry)
    {
        FreeNode *const node(reinterpret_cast<FreeNode *>(entry));

        node->mNext = mHead;
        mHead = node;
        ++mCount;
    }

private:

    /// Node in a list free of free entries within a block.
    struct FreeNode
    {
        FreeNode *mNext;        ///< Pointer to the next node in the free list.
    };

    FreeNode *mHead;            ///< Pointer to the first in a singly-linked list of free entries.
    uint32_t mCount;            ///< Number of entries on this free list.
};


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_PAGEDPOOL_FREELIST_H

