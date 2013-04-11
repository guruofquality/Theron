// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_DETAIL_ALLOCATORS_CACHINGALLOCATOR_H
#define THERON_DETAIL_ALLOCATORS_CACHINGALLOCATOR_H


#include <Theron/Align.h>
#include <Theron/Assert.h>
#include <Theron/BasicTypes.h>
#include <Theron/Defines.h>
#include <Theron/IAllocator.h>

#include <Theron/Detail/Allocators/Pool.h>
#include <Theron/Detail/Allocators/TrivialAllocator.h>


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
A caching allocator that caches free memory blocks of various small sizes.
*/
template <uint32_t POOL_COUNT, class LockType = NullCachingAllocatorLock>
class CachingAllocator : public Theron::IAllocator
{
public:

    /**
    Default constructor.
    Constructs a CachingAllocator around an internally owned Detail::TrivialAllocator.
    The TrivialAllocator acts as a trivial wrapper around global new and delete. The
    CachingAllocator adds caching of small allocations.
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

    /**
    Hashes a block size to a pool index.
    */
    inline static uint32_t MapBlockSizeToPool(const uint32_t size);

    CachingAllocator(const CachingAllocator &other);
    CachingAllocator &operator=(const CachingAllocator &other);

    inline void *AllocateInline(const uint32_t size, const uint32_t alignment);
    inline void FreeInline(void *const block, const uint32_t size);

    TrivialAllocator mTrivialAllocator;     ///< Default low-level allocator implementation.
    IAllocator *const mAllocator;           ///< Pointer to a wrapped low-level allocator.
    Pool<LockType> mPools[POOL_COUNT];      ///< Pools of memory blocks of different sizes.
};


template <uint32_t POOL_COUNT, class LockType>
THERON_FORCEINLINE CachingAllocator<POOL_COUNT, LockType>::CachingAllocator() :
  mTrivialAllocator(),
  mAllocator(&mTrivialAllocator)
{
}


template <uint32_t POOL_COUNT, class LockType>
THERON_FORCEINLINE CachingAllocator<POOL_COUNT, LockType>::CachingAllocator(IAllocator *const allocator) :
  mTrivialAllocator(),
  mAllocator(allocator)
{
}


template <uint32_t POOL_COUNT, class LockType>
THERON_FORCEINLINE CachingAllocator<POOL_COUNT, LockType>::~CachingAllocator()
{
    Clear();
}


template <uint32_t POOL_COUNT, class LockType>
inline void *CachingAllocator<POOL_COUNT, LockType>::Allocate(const uint32_t size)
{
    // Promote small allocations to cache-line size and alignment to improve cache hit rate.
    const uint32_t effectiveSize(size > THERON_CACHELINE_ALIGNMENT ? size : THERON_CACHELINE_ALIGNMENT);
    const uint32_t effectiveAlignment(THERON_CACHELINE_ALIGNMENT);

    // Force-inlined call.
    return AllocateInline(effectiveSize, effectiveAlignment);
}


template <uint32_t POOL_COUNT, class LockType>
inline void *CachingAllocator<POOL_COUNT, LockType>::AllocateAligned(const uint32_t size, const uint32_t alignment)
{
    // Promote small alignments to cache-line size and alignment to improve cache hit rate.
    const uint32_t effectiveSize(size > THERON_CACHELINE_ALIGNMENT ? size : THERON_CACHELINE_ALIGNMENT);
    const uint32_t effectiveAlignment(alignment > THERON_CACHELINE_ALIGNMENT ? alignment : THERON_CACHELINE_ALIGNMENT);

    // Force-inlined call.
    return AllocateInline(effectiveSize, effectiveAlignment);
}


template <uint32_t POOL_COUNT, class LockType>
THERON_FORCEINLINE void *CachingAllocator<POOL_COUNT, LockType>::AllocateInline(const uint32_t size, const uint32_t alignment)
{
    // Sizes are expected to be at least a cache-line.
    // Alignment values are expected to be powers of two and at least cache-line boundaries.
    THERON_ASSERT(size >= THERON_CACHELINE_ALIGNMENT);
    THERON_ASSERT(alignment >= THERON_CACHELINE_ALIGNMENT);
    THERON_ASSERT((alignment & (alignment - 1)) == 0);

    // Find the index of the pool containing blocks of this size.
    const uint32_t poolIndex(MapBlockSizeToPool(size));

    // We can't cache blocks bigger than a certain maximum size.
    if (poolIndex < POOL_COUNT)
    {
        // Search the pool for a block of the right alignment.
        Pool<LockType> &pool(mPools[poolIndex]);

        pool.Lock();
        void *const block(pool.FetchAligned(alignment));
        pool.Unlock();

        if (block)
        {
            return block;
        }
    }

    // We didn't find a cached block so allocate one with the wrapped allocator.
    return mAllocator->AllocateAligned(size, alignment);
}


template <uint32_t POOL_COUNT, class LockType>
inline void CachingAllocator<POOL_COUNT, LockType>::Free(void *const block)
{
    // This caching allocator relies on knowing the sizes of freed blocks.
    // We know the allocated size is at least a cache-line because we promote smaller alignments.
    // This does assume the memory was allocated using this cache, or another instance of it.
    // In the case where the actual memory block was larger than a cache line we waste the extra.
    const uint32_t effectiveSize(THERON_CACHELINE_ALIGNMENT);

    FreeInline(block, effectiveSize);
}


template <uint32_t POOL_COUNT, class LockType>
inline void CachingAllocator<POOL_COUNT, LockType>::Free(void *const block, const uint32_t size)
{
    // We know the allocated size is at least a cache-line because we promote smaller alignments.
    // This does assume the memory was allocated using this cache, or another instance of it.
    const uint32_t effectiveSize(size > THERON_CACHELINE_ALIGNMENT ? size : THERON_CACHELINE_ALIGNMENT);

    FreeInline(block, effectiveSize);
}


template <uint32_t POOL_COUNT, class LockType>
THERON_FORCEINLINE void CachingAllocator<POOL_COUNT, LockType>::FreeInline(void *const block, const uint32_t size)
{
    // Sizes are expected to be at least a cache-line.
    // Alignment values are expected to be powers of two and at least cache-line boundaries.
    THERON_ASSERT(size >= THERON_CACHELINE_ALIGNMENT);
    THERON_ASSERT(block);

    // Find the index of the pool containing blocks of this size.
    const uint32_t poolIndex(MapBlockSizeToPool(size));

    // We can't cache blocks bigger than a certain maximum size.
    if (poolIndex < POOL_COUNT)
    {
        // Add the block to the pool, if there is space left in the pool.
        Pool<LockType> &pool(mPools[poolIndex]);

        pool.Lock();
        const bool freed(pool.Add(block));
        pool.Unlock();

        if (freed)
        {
            return;
        }
    }

    // Can't cache this block; return it to the wrapped low-level allocator.
    mAllocator->Free(block);
}


template <uint32_t POOL_COUNT, class LockType>
THERON_FORCEINLINE void CachingAllocator<POOL_COUNT, LockType>::Clear()
{
    // Free any remaining blocks in the pools.
    for (uint32_t index = 0; index < POOL_COUNT; ++index)
    {
        Pool<LockType> &pool(mPools[index]);

        while (!pool.Empty())
        {
            void *const block = pool.Fetch();

            THERON_ASSERT(block);
            mAllocator->Free(block);
        }
    }
}


template <uint32_t POOL_COUNT, class LockType>
THERON_FORCEINLINE uint32_t CachingAllocator<POOL_COUNT, LockType>::MapBlockSizeToPool(const uint32_t size)
{
    // We assume that all allocations are multiples of four bytes.
    // Because we promote small allocations, the minimum size is a cache-line.
    THERON_ASSERT(size >= THERON_CACHELINE_ALIGNMENT);
    THERON_ASSERT((size & 3) == 0);

    // Because all allocation sizes are multiples of four, we divide by four.
    const uint32_t index(size >> 2);

    // Because the minimum size is a cache-line, we subtract the number of four-byte words in a cache-line.
    const uint32_t wordsPerCacheLine(THERON_CACHELINE_ALIGNMENT >> 2);
    THERON_ASSERT(index >= wordsPerCacheLine);
    return index - wordsPerCacheLine;
}


} // namespace Detail
} // namespace Theron


#ifdef _MSC_VER
#pragma warning(pop)
#endif //_MSC_VER


#endif // THERON_DETAIL_ALLOCATORS_CACHINGALLOCATOR_H
