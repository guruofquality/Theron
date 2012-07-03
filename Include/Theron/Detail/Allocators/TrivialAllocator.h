// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_DETAIL_ALLOCATORS_TRIVIALALLOCATOR_H
#define THERON_DETAIL_ALLOCATORS_TRIVIALALLOCATOR_H


#include <Theron/BasicTypes.h>
#include <Theron/IAllocator.h>

#include <Theron/Detail/Debug/Assert.h>


namespace Theron
{
namespace Detail
{


/**
\brief A trivial memory allocator implementing Theron::IAllocator.

All allocation and free requests are deferred to global new and delete.
Explicit non-default alignment is not supported.
*/
class TrivialAllocator : public IAllocator
{
public:

    /**
    \brief Default constructor
    */
    inline TrivialAllocator();

    /**
    \brief Virtual destructor.
    */
    inline virtual ~TrivialAllocator();

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

    TrivialAllocator(const TrivialAllocator &other);
    TrivialAllocator &operator=(const TrivialAllocator &other);
};


inline TrivialAllocator::TrivialAllocator()
{
}


inline TrivialAllocator::~TrivialAllocator()
{
}


inline void *TrivialAllocator::Allocate(const SizeType size)
{
    THERON_ASSERT(size > 4);
    return new uint8_t[size];
}


inline void *TrivialAllocator::AllocateAligned(const SizeType size, const SizeType /*alignment*/)
{
    // Alignment is not supported by this allocator.
    THERON_ASSERT(size > 4);
    return new uint8_t[size];
}


inline void TrivialAllocator::Free(void *const memory)
{
    THERON_ASSERT(memory);
    delete [] reinterpret_cast<uint8_t *>(memory);
}


inline void TrivialAllocator::Free(void *const memory, const SizeType /*size*/)
{
    // The size parameter is ignored.
    THERON_ASSERT(memory);
    delete [] reinterpret_cast<uint8_t *>(memory);
}


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_ALLOCATORS_TRIVIALALLOCATOR_H

