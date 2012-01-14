// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_DETAIL_PAGEDPOOL_PAGEDPOOL_H
#define THERON_DETAIL_PAGEDPOOL_PAGEDPOOL_H


#include <new>

#include <Theron/Detail/BasicTypes.h>
#include <Theron/Detail/Debug/Assert.h>
#include <Theron/Detail/PagedPool/FreeList.h>
#include <Theron/Detail/PagedPool/Page.h>

#include <Theron/AllocatorManager.h>
#include <Theron/IAllocator.h>
#include <Theron/Defines.h>


namespace Theron
{
namespace Detail
{


/// A growable pool in which objects can be allocated.
template <class Entry, uint32_t MAX_ENTRIES>
class PagedPool
{
public:

    /// Default Constructor.
    inline PagedPool() : mEntryCount(0), mMaxPageIndex(0)
    {
    }

    /// Returns the number of allocated entries.
    THERON_FORCEINLINE uint32_t Count() const
    {
        return mEntryCount;
    }

    /// Allocates an entity in the pool and returns its unique index.
    THERON_FORCEINLINE bool Allocate(uint32_t &index)
    {
        uint32_t pageIndex(0);
        uint32_t entryIndex(0);

        if (mEntryCount < MAX_ENTRIES)
        {
            // Check each initialized page in turn for a free entry.
            while (pageIndex < MAX_PAGES)
            {
                FreeList &freeList(mFreeLists[pageIndex]);
                PageType &page(mPageTable[pageIndex]);

                if (!page.IsInitialized())
                {
                    break;
                }

                if (page.Allocate(freeList, entryIndex))
                {
                    index = Index(pageIndex, entryIndex);
                    ++mEntryCount;

                    // Update the maximum page index.
                    if (pageIndex > mMaxPageIndex)
                    {
                        mMaxPageIndex = pageIndex;
                    }

                    return true;
                }

                ++pageIndex;
            }

            // All allocated pages are full. Initialize the first uninitialized page.
            if (pageIndex < MAX_PAGES)
            {
                FreeList &freeList(mFreeLists[pageIndex]);
                PageType &page(mPageTable[pageIndex]);

                if (page.Initialize(freeList))
                {
                    if (page.Allocate(freeList, entryIndex))
                    {
                        index = Index(pageIndex, entryIndex);
                        ++mEntryCount;
                        return true;
                    }
                }
            }
        }

        return false;
    }

    /// Frees the entry at the given index and returns its memory to the pool.
    THERON_FORCEINLINE bool Free(const uint32_t index)
    {
        const uint32_t pageIndex(PageIndex(index));
        const uint32_t entryIndex(EntryIndex(index));

        THERON_ASSERT(pageIndex < MAX_PAGES);

        // Since we're being asked to free the memory the page should exist!
        FreeList &freeList(mFreeLists[pageIndex]);
        PageType &page(mPageTable[pageIndex]);

        THERON_ASSERT(page.IsInitialized());
        if (page.Free(freeList, entryIndex))
        {
            --mEntryCount;

            // If the page has become unused then deallocate it.
            if (freeList.Count() == ENTRIES_PER_PAGE)
            {
                page.Release(freeList);
            }

            return true;
        }

        return false;
    }

    /// Gets a pointer to the entry at the given index.
    THERON_FORCEINLINE void *GetEntry(const uint32_t index) const
    {
        const uint32_t pageIndex(PageIndex(index));
        const uint32_t entryIndex(EntryIndex(index));

        THERON_ASSERT(pageIndex < MAX_PAGES);

        // If the address is stale then the page may not even exist any more.
        const PageType &page(mPageTable[pageIndex]);
        if (page.IsInitialized())
        {
            return page.GetEntry(entryIndex);
        }

        return 0;
    }

    /// Gets the index of the entry addressed by the given pointer.
    /// Returns MAX_ENTRIES if the entry is not found.
    THERON_FORCEINLINE uint32_t GetIndex(void *const entry) const
    {
        THERON_ASSERT(entry);

        // Search all pages that have ever been allocated for one which contains the entry.
        uint32_t pageIndex(0);
        while (pageIndex <= mMaxPageIndex)
        {
            const PageType &page(mPageTable[pageIndex]);
            if (page.IsInitialized())
            {
                const uint32_t entryIndex(page.GetIndex(entry));
                if (entryIndex < ENTRIES_PER_PAGE)
                {
                    return Index(pageIndex, entryIndex);
                }
            }

            ++pageIndex;
        }

        return MAX_ENTRIES;
    }

private:

    static const uint32_t ENTRIES_PER_PAGE = 64;
    static const uint32_t MAX_PAGES = (MAX_ENTRIES + ENTRIES_PER_PAGE - 1) / ENTRIES_PER_PAGE;
    static const uint32_t ENTRY_INDEX_MASK = ENTRIES_PER_PAGE - 1;
    static const uint32_t PAGE_INDEX_MASK = ~ENTRY_INDEX_MASK;
    static const uint32_t PAGE_INDEX_SHIFT = 6;

    typedef Page<Entry, ENTRIES_PER_PAGE> PageType;

    THERON_FORCEINLINE static uint32_t PageIndex(const uint32_t index)
    {
        return ((index & PAGE_INDEX_MASK) >> PAGE_INDEX_SHIFT);
    }

    THERON_FORCEINLINE static uint32_t EntryIndex(const uint32_t index)
    {
        return (index & ENTRY_INDEX_MASK);
    }

    THERON_FORCEINLINE static uint32_t Index(const uint32_t pageIndex, const uint32_t entryIndex)
    {
        return ((pageIndex << PAGE_INDEX_SHIFT) | entryIndex);
    }

    PagedPool(const PagedPool &other);
    PagedPool &operator=(const PagedPool &other);

    PageType mPageTable[MAX_PAGES];     ///< Table of allocated pages, each of which is basically a pointer to a buffer.
    FreeList mFreeLists[MAX_PAGES];     ///< Each page has its own dedicated list of free entries, so we can favour low-index pages.
    uint32_t mEntryCount;               ///< Number of allocated entries in the entire pool.
    uint32_t mMaxPageIndex;             ///< Maximum index of any allocated page.
};


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_PAGEDPOOL_PAGEDPOOL_H

