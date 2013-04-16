// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_DETAIL_ALLOCATORS_CACHINGALLOCATOR_H
#define THERON_DETAIL_ALLOCATORS_CACHINGALLOCATOR_H


#include <Theron/Align.h>
#include <Theron/Assert.h>
#include <Theron/BasicTypes.h>
#include <Theron/Defines.h>
#include <Theron/IAllocator.h>

#include <Theron/Detail/Allocators/Pool.h>


#ifdef _MSC_VER
#pragma warning(push)
#pragma warning (disable:4324)  // structure was padded due to __declspec(align())
#endif //_MSC_VER


namespace Theron
{
namespace Detail
{


class NullCachingAllocatorLock
{
public:

    THERON_FORCEINLINE void Lock()
    {
    }

    THERON_FORCEINLINE void Unlock()
    {
    }
};


/**
A thread-safe caching allocator that caches free memory blocks.
*/
template <uint32_t MAX_POOLS, uint32_t MAX_BLOCKS, class LockType = NullCachingAllocatorLock>
class CachingAllocator : public Theron::IAllocator
{
public:

    /**
    Default constructor.
    Constructs an uninitialized CachingAllocator referencing no lower-level allocator.
    */
    inline CachingAllocator();

    /**
    Explicit constructor.
    Constructs a CachingAllocator around an externally owned lower-level allocator.
    The CachingAllocator adds caching of small allocations.
    \param allocator Pointer to a lower-level allocator which the cache will wrap.
    */
    inline explicit CachingAllocator(IAllocator *const allocator);

    /**
    Destructor.
    */
    inline virtual ~CachingAllocator();

    /**
    Sets the internal allocator which is wrapped, or cached, by the caching allocator.
    \note This should only be called at start-of-day before any calls to Allocate.
    */
    inline void SetAllocator(IAllocator *const allocator);

    /**
    Allocates a memory block of the given size.
    */
    inline virtual void *Allocate(const uint32_t size);

    /**
    Allocates a memory block of the given size and alignment.
    */
    inline virtual void *AllocateAligned(const uint32_t size, const uint32_t alignment);

    /**
    Frees a previously allocated memory block.
    */
    inline virtual void Free(void *const block);

    /**
    Frees a previously allocated memory block of a known size.
    */
    inline virtual void Free(void *const block, const uint32_t size);

    /**
    Frees all currently cached memory blocks.
    */
    inline void Clear();

private:

    class Entry
    {
    public:

        typedef Detail::Pool<MAX_BLOCKS> PoolType;

        inline Entry() : mBlockSize(0)
        {
        }

        uint32_t mBlockSize;
        PoolType mPool;
    };

    CachingAllocator(const CachingAllocator &other);
    CachingAllocator &operator=(const CachingAllocator &other);

    inline void *AllocateInline(const uint32_t size, const uint32_t alignment);
    inline void FreeInline(void *const block, const uint32_t size);

    IAllocator *mAllocator;         ///< Pointer to a wrapped low-level allocator.
    LockType mMutex;                ///< Protects access to the pools.
    Entry mEntries[MAX_POOLS];      ///< Pools of memory blocks of different sizes.
};


template <uint32_t MAX_POOLS, uint32_t MAX_BLOCKS, class LockType>
THERON_FORCEINLINE CachingAllocator<MAX_POOLS, MAX_BLOCKS, LockType>::CachingAllocator() : mAllocator(0)
{
}


template <uint32_t MAX_POOLS, uint32_t MAX_BLOCKS, class LockType>
THERON_FORCEINLINE CachingAllocator<MAX_POOLS, MAX_BLOCKS, LockType>::CachingAllocator(IAllocator *const allocator) : mAllocator(allocator)
{
}


template <uint32_t MAX_POOLS, uint32_t MAX_BLOCKS, class LockType>
THERON_FORCEINLINE CachingAllocator<MAX_POOLS, MAX_BLOCKS, LockType>::~CachingAllocator()
{
    Clear();
}


template <uint32_t MAX_POOLS, uint32_t MAX_BLOCKS, class LockType>
inline void CachingAllocator<MAX_POOLS, MAX_BLOCKS, LockType>::SetAllocator(IAllocator *const allocator)
{
    mAllocator = allocator;
}


template <uint32_t MAX_POOLS, uint32_t MAX_BLOCKS, class LockType>
inline void *CachingAllocator<MAX_POOLS, MAX_BLOCKS, LockType>::Allocate(const uint32_t size)
{
    // Promote small allocations to cache-line size and alignment to improve cache hit rate.
    const uint32_t effectiveSize(size > THERON_CACHELINE_ALIGNMENT ? size : THERON_CACHELINE_ALIGNMENT);
    const uint32_t effectiveAlignment(THERON_CACHELINE_ALIGNMENT);

    // Force-inlined call.
    return AllocateInline(effectiveSize, effectiveAlignment);
}


template <uint32_t MAX_POOLS, uint32_t MAX_BLOCKS, class LockType>
inline void *CachingAllocator<MAX_POOLS, MAX_BLOCKS, LockType>::AllocateAligned(const uint32_t size, const uint32_t alignment)
{
    // Promote small alignments to cache-line size and alignment to improve cache hit rate.
    const uint32_t effectiveSize(size > THERON_CACHELINE_ALIGNMENT ? size : THERON_CACHELINE_ALIGNMENT);
    const uint32_t effectiveAlignment(alignment > THERON_CACHELINE_ALIGNMENT ? alignment : THERON_CACHELINE_ALIGNMENT);

    // Force-inlined call.
    return AllocateInline(effectiveSize, effectiveAlignment);
}


template <uint32_t MAX_POOLS, uint32_t MAX_BLOCKS, class LockType>
THERON_FORCEINLINE void *CachingAllocator<MAX_POOLS, MAX_BLOCKS, LockType>::AllocateInline(const uint32_t size, const uint32_t alignment)
{
    void *block(0);

    // Sizes are expected to be at least a cache-line.
    // Alignment values are expected to be powers of two and at least cache-line boundaries.
    THERON_ASSERT(size >= THERON_CACHELINE_ALIGNMENT);
    THERON_ASSERT(alignment >= THERON_CACHELINE_ALIGNMENT);
    THERON_ASSERT((alignment & (alignment - 1)) == 0);

    mMutex.Lock();

    // The last pool is reserved and should always be empty.
    THERON_ASSERT(mEntries[MAX_POOLS - 1].mBlockSize == 0);
    THERON_ASSERT(mEntries[MAX_POOLS - 1].mPool.Empty());

    // Search each entry in turn for one whose pool contains blocks of the required size.
    // Stop if we reach the first unused entry (marked by a block size of zero).
    uint32_t index(0);
    while (index < MAX_POOLS)
    {
        Entry &entry(mEntries[index]);
        if (entry.mBlockSize == size)
        {
            // Try to allocate a block from the pool.
            block = entry.mPool.FetchAligned(alignment);
            break;
        }

        // Found the first unused pool or the very last pool?
        if (entry.mBlockSize == 0)
        {
            // Reserve it for blocks of the current size.
            THERON_ASSERT(entry.mPool.Empty());
            entry.mBlockSize = size;
            break;
        }

        ++index;
    }

    // Swap the pool with that of lower index, if it isn't already first.
    // This is a kind of least-recently-requested replacement policy for pools.
    if (index > 0)
    {
        Entry temp(mEntries[index]);
        mEntries[index] = mEntries[index - 1];
        mEntries[index - 1] = temp;
    }

    // If we claimed the last pool then clear the pool which is
    // now last after the swap, so it's available for next time.
    if (index == MAX_POOLS - 1)
    {
        Entry &entry(mEntries[MAX_POOLS - 1]);
        while (!entry.mPool.Empty())
        {
            mAllocator->Free(entry.mPool.Fetch(), entry.mBlockSize);
        }
    }

    // Check that the last pool has been left unused and empty.
    THERON_ASSERT(mEntries[MAX_POOLS - 1].mBlockSize == 0);
    THERON_ASSERT(mEntries[MAX_POOLS - 1].mPool.Empty());

    mMutex.Unlock();

    if (block == 0)
    {
        // Allocate a new block.
        block = mAllocator->AllocateAligned(size, alignment);
    }

    return block;
}


template <uint32_t MAX_POOLS, uint32_t MAX_BLOCKS, class LockType>
inline void CachingAllocator<MAX_POOLS, MAX_BLOCKS, LockType>::Free(void *const block)
{
    // This caching allocator relies on knowing the sizes of freed blocks.
    // We know the allocated size is at least a cache-line because we promote smaller alignments.
    // This does assume the memory was allocated using this cache, or another instance of it.
    // In the case where the actual memory block was larger than a cache line we waste the extra.
    const uint32_t effectiveSize(THERON_CACHELINE_ALIGNMENT);

    FreeInline(block, effectiveSize);
}


template <uint32_t MAX_POOLS, uint32_t MAX_BLOCKS, class LockType>
inline void CachingAllocator<MAX_POOLS, MAX_BLOCKS, LockType>::Free(void *const block, const uint32_t size)
{
    // We know the allocated size is at least a cache-line because we promote smaller alignments.
    // This does assume the memory was allocated using this cache, or another instance of it.
    const uint32_t effectiveSize(size > THERON_CACHELINE_ALIGNMENT ? size : THERON_CACHELINE_ALIGNMENT);

    FreeInline(block, effectiveSize);
}


template <uint32_t MAX_POOLS, uint32_t MAX_BLOCKS, class LockType>
THERON_FORCEINLINE void CachingAllocator<MAX_POOLS, MAX_BLOCKS, LockType>::FreeInline(void *const block, const uint32_t size)
{
    bool added(false);

    // Sizes are expected to be at least a cache-line.
    // Alignment values are expected to be powers of two and at least cache-line boundaries.
    THERON_ASSERT(size >= THERON_CACHELINE_ALIGNMENT);
    THERON_ASSERT(block);

    mMutex.Lock();

    // Search each entry in turn for one whose pool is for blocks of the given size.
    // Stop if we reach the first unused entry (marked by a block size of zero).
    // Don't search the very last pool, since it is reserved for use in swaps.
    uint32_t index(0);
    while (index < MAX_POOLS && mEntries[index].mBlockSize)
    {
        Entry &entry(mEntries[index]);
        if (entry.mBlockSize == size)
        {
            // Try to add the block to the pool, if it's not already full.
            added = entry.mPool.Add(block);
            break;
        }

        ++index;
    }

    mMutex.Unlock();

    if (!added)
    {
        // No pools are assigned to blocks of this size; free it.
        mAllocator->Free(block, size);
    }
}


template <uint32_t MAX_POOLS, uint32_t MAX_BLOCKS, class LockType>
THERON_FORCEINLINE void CachingAllocator<MAX_POOLS, MAX_BLOCKS, LockType>::Clear()
{
    mMutex.Lock();

    // Free any remaining blocks in the pools.
    for (uint32_t index = 0; index < MAX_POOLS; ++index)
    {
        Entry &entry(mEntries[index]);
        while (!entry.mPool.Empty())
        {
            mAllocator->Free(entry.mPool.Fetch());
        }
    }

    mMutex.Unlock();
}


} // namespace Detail
} // namespace Theron


#ifdef _MSC_VER
#pragma warning(pop)
#endif //_MSC_VER


#endif // THERON_DETAIL_ALLOCATORS_CACHINGALLOCATOR_H
