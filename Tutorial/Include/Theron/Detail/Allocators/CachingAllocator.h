// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_DETAIL_ALLOCATORS_CACHINGALLOCATOR_H
#define THERON_DETAIL_ALLOCATORS_CACHINGALLOCATOR_H


#include <Theron/Assert.h>
#include <Theron/BasicTypes.h>
#include <Theron/Defines.h>
#include <Theron/IAllocator.h>

#include <Theron/Detail/Allocators/Pool.h>
#include <Theron/Detail/Allocators/TrivialAllocator.h>


namespace Theron
{
namespace Detail
{


/**
A caching allocator that caches free memory blocks of various small sizes.
*/
template <uint32_t POOL_COUNT>
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

    TrivialAllocator mTrivialAllocator;     ///< Default low-level allocator implementation.
    IAllocator *const mAllocator;           ///< Pointer to a wrapped low-level allocator.
    Pool mPools[POOL_COUNT];                ///< Pools of memory blocks of different sizes.
};


template <uint32_t POOL_COUNT>
THERON_FORCEINLINE CachingAllocator<POOL_COUNT>::CachingAllocator() :
  mTrivialAllocator(),
  mAllocator(&mTrivialAllocator)
{
}


template <uint32_t POOL_COUNT>
THERON_FORCEINLINE CachingAllocator<POOL_COUNT>::CachingAllocator(IAllocator *const allocator) :
  mTrivialAllocator(),
  mAllocator(allocator)
{
}


template <uint32_t POOL_COUNT>
THERON_FORCEINLINE CachingAllocator<POOL_COUNT>::~CachingAllocator()
{
    Clear();
}


template <uint32_t POOL_COUNT>
inline void *CachingAllocator<POOL_COUNT>::Allocate(const uint32_t size)
{
    // Default to 4-byte alignment.
    return AllocateAligned(size, 4);
}


template <uint32_t POOL_COUNT>
inline void *CachingAllocator<POOL_COUNT>::AllocateAligned(const uint32_t size, const uint32_t alignment)
{
    // Alignment values are expected to be powers of two.
    THERON_ASSERT(size);
    THERON_ASSERT(alignment);
    THERON_ASSERT((alignment & (alignment - 1)) == 0);

    // Find the index of the pool containing blocks of this size.
    const uint32_t poolIndex(MapBlockSizeToPool(size));

    // We can't cache blocks bigger than a certain maximum size.
    if (poolIndex < POOL_COUNT)
    {
        // Search the pool for a block of the right alignment.
        Pool &pool(mPools[poolIndex]);
        if (void *const block = pool.FetchAligned(alignment))
        {
            return block;
        }
    }

    // We didn't find a cached block so allocate a one with the wrapped allocator.
    return mAllocator->AllocateAligned(size, alignment);
}


template <uint32_t POOL_COUNT>
inline void CachingAllocator<POOL_COUNT>::Free(void *const block)
{
    // Can't cache this block; return it to the wrapper low-level allocator.
    // This caching allocator relies on knowing the sizes of freed blocks.
    mAllocator->Free(block);
}


template <uint32_t POOL_COUNT>
inline void CachingAllocator<POOL_COUNT>::Free(void *const block, const uint32_t size)
{
    THERON_ASSERT(block);
    THERON_ASSERT(size);

    // Find the index of the pool containing blocks of this size.
    const uint32_t poolIndex(MapBlockSizeToPool(size));

    // We can't cache blocks bigger than a certain maximum size.
    if (poolIndex < POOL_COUNT)
    {
        // Add the block to the pool, if there is space left in the pool.
        Pool &pool(mPools[poolIndex]);
        if (pool.Add(block))
        {
            return;
        }
    }

    // Can't cache this block; return it to the wrapper low-level allocator.
    mAllocator->Free(block);
}


template <uint32_t POOL_COUNT>
THERON_FORCEINLINE void CachingAllocator<POOL_COUNT>::Clear()
{
    // Free any remaining blocks in the pools.
    for (uint32_t index = 0; index < POOL_COUNT; ++index)
    {
        Pool &pool(mPools[index]);

        while (!pool.Empty())
        {
            void *const block = pool.Fetch();

            THERON_ASSERT(block);
            mAllocator->Free(block);
        }
    }
}


template <uint32_t POOL_COUNT>
THERON_FORCEINLINE uint32_t CachingAllocator<POOL_COUNT>::MapBlockSizeToPool(const uint32_t size)
{
    // We assume that all allocations are non-zero multiples of four bytes!
    THERON_ASSERT(size >= 4);
    THERON_ASSERT((size & 3) == 0);

    // Because all allocation sizes are multiples of four, we divide by four.
    const uint32_t index(size >> 2);

    // Because the minimum size is four bytes, we subtract one.
    THERON_ASSERT(index > 0);
    return index - 1;
}


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_ALLOCATORS_CACHINGALLOCATOR_H
