// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef COMMON_LINEARALLOCATOR_H
#define COMMON_LINEARALLOCATOR_H


#include <Theron/Detail/Debug/Assert.h>
#include <Theron/Detail/Threading/Lock.h>
#include <Theron/Detail/Threading/Mutex.h>

#include <Theron/Align.h>
#include <Theron/IAllocator.h>


namespace Example
{


// A simple linear allocator implementing Theron::IAllocator.
// The allocator is constructed around a memory buffer, and just dishes it out.
// It correctly supports aligned allocations.
class LinearAllocator : public Theron::IAllocator
{
public:

    // Constructs a linear allocator around a memory buffer.
    inline LinearAllocator(void *const buffer, const SizeType size) :
      mBuffer(static_cast<unsigned char *>(buffer)),
      mOffset(mBuffer),
      mEnd(mBuffer + size),
      mMutex()
    {
    }

    // Virtual destructor
    inline virtual ~LinearAllocator()
    {
    }

    // Allocates a piece of contiguous memory.
    inline virtual void *Allocate(const SizeType size)
    {
        // Allocate with default 4-byte alignment.
        return AllocateAligned(size, 4);
    }

    // Allocates a piece of contiguous memory aligned at a specific byte-multiple boundary.
    inline virtual void *AllocateAligned(const SizeType size, const SizeType alignment)
    {
        unsigned char *allocation(0);
        Theron::Detail::Lock lock(mMutex);

        allocation = mOffset;
        THERON_ALIGN(allocation, alignment);

        if (allocation + size <= mEnd)
        {
            mOffset = allocation + size;
        }
        else
        {
            allocation  = 0;
        }

        return static_cast<void *>(allocation);
    }

    // Frees a previously allocated piece of contiguous memory.
    inline virtual void Free(void *const /*memory*/)
    {
        // Nothing to do
    }    

    // Returns the total amount of free space remaining.
    inline SizeType FreeSpace()
    {
        return static_cast<SizeType>(mEnd - mOffset);
    }    

private:

    LinearAllocator(const LinearAllocator &other);
    LinearAllocator &operator=(const LinearAllocator &other);

    unsigned char *mBuffer;        // Base address of referenced memory buffer.
    unsigned char *mOffset;        // Current place within referenced memory buffer.
    unsigned char *mEnd;           // End of referenced memory buffer (exclusive).
    Theron::Detail::Mutex mMutex;  // Mutex for thread synchronization.
};


} // namespace Example


#endif // COMMON_LINEARALLOCATOR_H

