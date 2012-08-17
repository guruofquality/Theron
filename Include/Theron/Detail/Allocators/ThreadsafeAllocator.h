// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_DETAIL_ALLOCATORS_THREADSAFEALLOCATOR_H
#define THERON_DETAIL_ALLOCATORS_THREADSAFEALLOCATOR_H


#include <Theron/Assert.h>
#include <Theron/IAllocator.h>

#include <Theron/Detail/Allocators/TrivialAllocator.h>
#include <Theron/Detail/Threading/SpinLock.h>


namespace Theron
{
namespace Detail
{


/**
\brief A simple thread-safe allocator implementing Theron::IAllocator.

All allocation and free requests are deferred to a lower-level allocator provided on construction.
*/
class ThreadsafeAllocator : public IAllocator
{
public:

    /**
    \brief Default constructor.
    Constructs a ThreadsafeAllocator around an internally owned Detail::TrivialAllocator.
    The TrivialAllocator acts as a trivial wrapper around global new and delete. The
    ThreadsafeAllocator adds thread-safety to the wrapped TrivialAllocator.
    */
    inline ThreadsafeAllocator();

    /**
    \brief Explicit constructor.
    Constructs a ThreadsafeAllocator around an externally owned lower-level allocator.
    ThreadsafeAllocator adds thread-safety to the wrapped lower-level allocator.
    \param allocator Pointer to a lower-level allocator which the allocator will wrap.
    */
    inline explicit ThreadsafeAllocator(IAllocator *const allocator);

    /**
    \brief Virtual destructor.
    */
    inline virtual ~ThreadsafeAllocator();

    /**
    \brief Allocates a block of contiguous memory.
    \param size The size of the memory block to allocate, in bytes, which must be a non-zero multiple of four bytes.
    \return A pointer to the allocated memory.
    */
    inline virtual void *Allocate(const SizeType size);

    /**
    \brief Allocates a block of contiguous memory aligned to a given byte-multiple boundary.

    \note This allocator doesn't support alignment. The requested alignment is ignored and the
    alignment of the allocated memory block is not guaranteed.

    \param size The size of the memory block to allocate, in bytes, which must be a non-zero multiple of four bytes.
    \param alignment The alignment of the memory to allocate, in bytes, which must be a power of two.
    \return A pointer to the allocated memory.
    */
    inline virtual void *AllocateAligned(const SizeType size, const SizeType alignment);

    /**
    \brief Frees a previously allocated block of contiguous memory.

    \param memory Pointer to the memory to be deallocated.
    \note The pointer must not be null, and must address an allocated block of memory.
    */
    inline virtual void Free(void *const memory);

    /**
    \brief Frees a previously allocated block of contiguous memory of a known size.

    \param memory Pointer to the memory to be deallocated.
    \param size The size of the freed memory block.

    \note The pointer must not be null, and must address an allocated block of memory.
    */
    inline virtual void Free(void *const memory, const SizeType size);

private:

    ThreadsafeAllocator(const ThreadsafeAllocator &other);
    ThreadsafeAllocator &operator=(const ThreadsafeAllocator &other);

    TrivialAllocator mTrivialAllocator;     ///< Default low-level allocator implementation.
    IAllocator *const mAllocator;           ///< Pointer to a wrapped low-level allocator.
    SpinLock mSpinLock;                     ///< Thread synchronization object providing thread-safety.
};


inline ThreadsafeAllocator::ThreadsafeAllocator() :
  mTrivialAllocator(),
  mAllocator(&mTrivialAllocator),
  mSpinLock()
{
}


inline ThreadsafeAllocator::ThreadsafeAllocator(IAllocator *const allocator) :
  mTrivialAllocator(),
  mAllocator(allocator),
  mSpinLock()
{
}


inline ThreadsafeAllocator::~ThreadsafeAllocator()
{
}


inline void *ThreadsafeAllocator::Allocate(const SizeType size)
{
    void *block(0);

    mSpinLock.Lock();
    block = mAllocator->Allocate(size);
    mSpinLock.Unlock();

    return block;
}


inline void *ThreadsafeAllocator::AllocateAligned(const SizeType size, const SizeType alignment)
{
    void *block(0);

    mSpinLock.Lock();
    block = mAllocator->AllocateAligned(size, alignment);
    mSpinLock.Unlock();

    return block;
}


inline void ThreadsafeAllocator::Free(void *const memory)
{
    mSpinLock.Lock();
    mAllocator->Free(memory);
    mSpinLock.Unlock();
}


inline void ThreadsafeAllocator::Free(void *const memory, const SizeType size)
{
    mSpinLock.Lock();
    mAllocator->Free(memory, size);
    mSpinLock.Unlock();
}


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_ALLOCATORS_THREADSAFEALLOCATOR_H
