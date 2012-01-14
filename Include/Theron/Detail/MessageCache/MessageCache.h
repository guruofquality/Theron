// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_DETAIL_MESSAGECACHE_MESSAGECACHE_H
#define THERON_DETAIL_MESSAGECACHE_MESSAGECACHE_H


#include <Theron/Detail/BasicTypes.h>
#include <Theron/Detail/Debug/Assert.h>
#include <Theron/Detail/MessageCache/Pool.h>
#include <Theron/Detail/Threading/Lock.h>
#include <Theron/Detail/Threading/Mutex.h>

#include <Theron/AllocatorManager.h>
#include <Theron/Defines.h>


namespace Theron
{
namespace Detail
{


/// A global cache of free message memory blocks of different sizes.
class MessageCache
{
public:

    /// Gets a reference to the single global instance.
    inline static MessageCache &Instance();

    /// Default constructor.
    inline MessageCache();

    /// Destructor.
    inline ~MessageCache();

    /// References the singleton instance.
    inline void Reference();

    /// Dereferences the singleton instance.
    /// Any cached memory blocks are freed on last dereference.
    inline void Dereference();

    /// Allocates a memory block of the given size.
    inline void *Allocate(const uint32_t size, const uint32_t alignment);

    /// Frees a previously allocated memory block.
    inline void Free(void *const block, const uint32_t size);

private:

    /// Hashes a block size to a pool index.
    inline static uint32_t MapBlockSizeToPool(const uint32_t size);

    MessageCache(const MessageCache &other);
    MessageCache &operator=(const MessageCache &other);

    /// Number of memory block pools maintained.
    /// Each pool holds memory blocks of a specific size.
    /// The number of pools dictates the maximum block size that can be cached.
    static const uint32_t MAX_POOLS = 32;

    static MessageCache smInstance;         ///< Single, static instance of the class.

    Mutex mReferenceCountMutex;             ///< Synchronizes access to the reference count.
    uint32_t mReferenceCount;               ///< Tracks how many clients exist.
    Pool mPools[MAX_POOLS];                 ///< Pools of memory blocks of different sizes.
};


THERON_FORCEINLINE MessageCache &MessageCache::Instance()
{
    return smInstance;
}


THERON_FORCEINLINE MessageCache::MessageCache() :
  mReferenceCountMutex(),
  mReferenceCount(0)
{
}


inline MessageCache::~MessageCache()
{
    // Check that the pools were all emptied when the cache became unreferenced.
    // If these asserts fail it probably means a Theron object (either an actor
    // or a receiver) wasn't destructed prior to the application ending.
    for (uint32_t index = 0; index < MAX_POOLS; ++index)
    {
        THERON_ASSERT(mPools[index].Empty());
    }
}


THERON_FORCEINLINE void MessageCache::Reference()
{
    Lock lock(mReferenceCountMutex);
    if (mReferenceCount++ == 0)
    {
        // Check that the pools were all left empty from the last use, if any.
        for (uint32_t index = 0; index < MAX_POOLS; ++index)
        {
            THERON_ASSERT(mPools[index].Empty());
        }
    }
}


THERON_FORCEINLINE void MessageCache::Dereference()
{
    Lock lock(mReferenceCountMutex);
    if (--mReferenceCount == 0)
    {
        // Free any remaining blocks in the pools.
        for (uint32_t index = 0; index < MAX_POOLS; ++index)
        {
            mPools[index].Clear();
        }
    }
}


THERON_FORCEINLINE void *MessageCache::Allocate(const uint32_t size, const uint32_t alignment)
{
    // Alignment values are expected to be powers of two.
    THERON_ASSERT(size);
    THERON_ASSERT(alignment);
    THERON_ASSERT((alignment & (alignment - 1)) == 0);

    // Find the index of the pool containing blocks of this size.
    const uint32_t poolIndex(MapBlockSizeToPool(size));

    // We can't cache blocks bigger than a certain maximum size.
    if (poolIndex < MAX_POOLS)
    {
        // Search the pool for a block of the right alignment.
        Pool &pool(mPools[poolIndex]);
        if (void *const block = pool.FetchAligned(alignment))
        {
            return block;
        }
    }

    // We didn't find a cached block so we need to allocate a new one from the user allocator.
    return AllocatorManager::Instance().GetAllocator()->AllocateAligned(size, alignment);
}


THERON_FORCEINLINE void MessageCache::Free(void *const block, const uint32_t size)
{
    THERON_ASSERT(block);
    THERON_ASSERT(size);

    // Find the index of the pool containing blocks of this size.
    const uint32_t poolIndex(MapBlockSizeToPool(size));

    // We can't cache blocks bigger than a certain maximum size.
    if (poolIndex < MAX_POOLS)
    {
        // Add the block to the pool, if there is space left in the pool.
        if (mPools[poolIndex].Add(block))
        {
            return;
        }
    }

    // Can't cache this block; return it to the user allocator.
    AllocatorManager::Instance().GetAllocator()->Free(block);
}


THERON_FORCEINLINE uint32_t MessageCache::MapBlockSizeToPool(const uint32_t size)
{
    // We assume that all allocations are non-zero multiples of four bytes!
    THERON_ASSERT((size & 3) == 0);

    // Because all allocation sizes are multiples of four, we divide by four.
    const uint32_t index(size >> 2);

    // Because the minimum size is four bytes, we subtract one.
    THERON_ASSERT(index > 0);
    return index - 1;
}


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_MESSAGECACHE_MESSAGECACHE_H

