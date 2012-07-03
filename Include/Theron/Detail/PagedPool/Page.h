// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_DETAIL_PAGEDPOOL_PAGE_H
#define THERON_DETAIL_PAGEDPOOL_PAGE_H


#include <Theron/AllocatorManager.h>
#include <Theron/IAllocator.h>
#include <Theron/BasicTypes.h>
#include <Theron/Defines.h>

#include <Theron/Detail/Debug/Assert.h>
#include <Theron/Detail/PagedPool/FreeList.h>


namespace Theron
{
namespace Detail
{


template <class Entry, uint32_t ENTRIES_PER_PAGE, uint32_t ENTRY_ALIGNMENT>
class Page
{
public:

    THERON_FORCEINLINE static uint32_t PaddedEntrySize()
    {
        const uint32_t entrySize(static_cast<uint32_t>(sizeof(Entry)));
        const uint32_t cacheLinesPerEntry((entrySize + ENTRY_ALIGNMENT - 1) / ENTRY_ALIGNMENT);
        return cacheLinesPerEntry * ENTRY_ALIGNMENT;
    }

    inline Page() : mData(0)
    {
    }

    inline ~Page()
    {
        // We expect the page to be unused and hence released.
        THERON_ASSERT(mData == 0);
    }

    /// Returns true if the page has been allocated by a successful call to Initialize.
    THERON_FORCEINLINE bool IsInitialized() const
    {
        return (mData != 0);
    }

    /// Initializes the page, allocating its data buffer and marking all entries as free.
    /// \return True, if the page data buffer was successfully allocated.
    inline bool Initialize(FreeList &freeList)
    {
        // Allocate the page data buffer.
        IAllocator *const allocator(AllocatorManager::Instance().GetAllocator());
        const uint32_t paddedEntrySize(PaddedEntrySize());
        const uint32_t pageSize(ENTRIES_PER_PAGE * paddedEntrySize);

        mData = reinterpret_cast<uint8_t *>(allocator->AllocateAligned(pageSize, ENTRY_ALIGNMENT));

        if (mData == 0)
        {
            return false;
        }

        // Add all the entries in the page to the free list initially.
        // We add them at the front of the list in reverse order so the list starts at the low end.
        uint8_t *const firstEntry(mData);
        uint8_t *const lastEntry(firstEntry + (ENTRIES_PER_PAGE - 1) * paddedEntrySize);

        uint8_t *entry(lastEntry);
        while (entry >= firstEntry)
        {
            // Add the free entry to the freelist.
            freeList.Add(entry);
            entry -= paddedEntrySize;
        }

        return true;
    }

    /// Releases the page, de-allocating its data buffer and clearing its free list.
    inline void Release(FreeList &freeList)
    {
        THERON_ASSERT(mData);

        IAllocator *const allocator(AllocatorManager::Instance().GetAllocator());
        allocator->Free(mData);
        mData = 0;

        freeList.Clear();
    }

    /// Allocates a free entry and sets its index, returning true on success.
    THERON_FORCEINLINE bool Allocate(FreeList &freeList, uint32_t &index)
    {
        void *const memory(freeList.Get());
        if (memory)
        {
            index = GetIndex(memory);
            return true;
        }

        return false;
    }

    /// Frees a previously allocated entry.
    THERON_FORCEINLINE bool Free(FreeList &freeList, const uint32_t index)
    {
        const uint32_t paddedEntrySize(PaddedEntrySize());

        THERON_ASSERT(mData);
        THERON_ASSERT(index < ENTRIES_PER_PAGE);

        // Calculate the address of the entry after padding.
        uint8_t *const firstEntry(mData);
        uint8_t *const entry(firstEntry + index * paddedEntrySize);

        THERON_ASSERT(THERON_ALIGNED(entry, ENTRY_ALIGNMENT));

        // Add the free entry to the freelist.
        freeList.Add(entry);

        return true;
    }

    /// Gets a pointer to the entry at the given index.
    THERON_FORCEINLINE void *GetEntry(const uint32_t index) const
    {
        const uint32_t paddedEntrySize(PaddedEntrySize());

        THERON_ASSERT(mData);
        THERON_ASSERT(index < ENTRIES_PER_PAGE);

        // Calculate the address of the entry after padding.
        uint8_t *const firstEntry(mData);
        uint8_t *const entry(firstEntry + index * paddedEntrySize);

        THERON_ASSERT(THERON_ALIGNED(entry, ENTRY_ALIGNMENT));

        return reinterpret_cast<void *>(entry);
    }

    /// Gets the index of the entry addressed by the given pointer.
    /// Returns ENTRIES_PER_PAGE if the pointer isn't the address of an entry in this page.
    THERON_FORCEINLINE uint32_t GetIndex(void *const ptr) const
    {
        const uint32_t paddedEntrySize(PaddedEntrySize());

        THERON_ASSERT(ptr);
        THERON_ASSERT(THERON_ALIGNED(ptr, ENTRY_ALIGNMENT));

        uint8_t *const entry(reinterpret_cast<uint8_t *>(ptr));
        uint8_t *const firstEntry(mData);
        uint8_t *const lastEntry(firstEntry + (ENTRIES_PER_PAGE - 1) * paddedEntrySize);

        // Entry within page?
        if (entry >= firstEntry && entry <= lastEntry)
        {
            const uint32_t entryOffset(static_cast<uint32_t>(entry - firstEntry));
            THERON_ASSERT((entryOffset % paddedEntrySize) == 0);

            return (entryOffset / paddedEntrySize);
        }

        return ENTRIES_PER_PAGE;
    }

private:

    uint8_t *mData;         ///< A page is just a pointer to an allocated buffer of padded entries.
};


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_PAGEDPOOL_PAGE_H

