// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#include <Theron/Assert.h>
#include <Theron/Defines.h>

#include <Theron/Detail/Strings/StringPool.h>
#include <Theron/Detail/Threading/Lock.h>


#ifdef _MSC_VER
#pragma warning(push)
#pragma warning (disable:4996)  // function or variable may be unsafe.
#endif //_MSC_VER


namespace Theron
{
namespace Detail
{


StringPool::Container *StringPool::smContainer = 0;
Mutex StringPool::smMutex;
uint32_t StringPool::smReferenceCount = 0;


void StringPool::Reference()
{
    Lock lock(smMutex);

    // Create the singleton instance if this is the first reference.
    if (smReferenceCount++ == 0)
    {
        IAllocator *const allocator(AllocatorManager::Instance().GetAllocator());
        void *const memory(allocator->AllocateAligned(sizeof(Container), THERON_CACHELINE_ALIGNMENT));
        smContainer = new (memory) Container();
    }
}


void StringPool::Dereference()
{
    Lock lock(smMutex);

    // Destroy the singleton instance if this was the last reference.
    if (--smReferenceCount == 0)
    {
        IAllocator *const allocator(AllocatorManager::Instance().GetAllocator());
        smContainer->~Container();
        allocator->Free(smContainer, sizeof(Container));
    }
}


StringPool::Container::Container()
{
}


StringPool::Container::~Container()
{
    IAllocator *const allocator(AllocatorManager::Instance().GetAllocator());

    // Free all entries.
    while (!mEntries.Empty())
    {
        Entry *const entry(mEntries.Front());
        mEntries.Remove(entry);

        entry->~Entry();
        allocator->Free(entry, sizeof(Entry));
    }
}


const char *StringPool::Container::Get(const char *const str)
{
    Lock lock(mMutex);

    // TODO: For now we linear search and strcmp every entry!
    // Search for an existing entry for this string.
    List<Entry>::Iterator entries(mEntries.GetIterator());
    while (entries.Next())
    {
        Entry *const entry(entries.Get());
        if (strcmp(entry->Value(), str) == 0)
        {
            return entry->Value();
        }
    }

    // Create a new entry.
    IAllocator *const allocator(AllocatorManager::Instance().GetAllocator());
    void *const memory(allocator->Allocate(sizeof(Entry)));
    Entry *const entry = new (memory) Entry(str);

    mEntries.Insert(entry);
    return entry->Value();
}


} // namespace Detail
} // namespace Theron


#ifdef _MSC_VER
#pragma warning(pop)
#endif //_MSC_VER
