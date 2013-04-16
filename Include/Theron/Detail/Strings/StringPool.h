// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_DETAIL_STRINGS_STRINGPOOL_H
#define THERON_DETAIL_STRINGS_STRINGPOOL_H


#include <new>

#include <string.h>
#include <stdlib.h>

#include <Theron/Align.h>
#include <Theron/AllocatorManager.h>
#include <Theron/Assert.h>
#include <Theron/BasicTypes.h>
#include <Theron/Defines.h>
#include <Theron/IAllocator.h>

#include <Theron/Detail/Containers/List.h>
#include <Theron/Detail/Threading/Mutex.h>


#ifdef _MSC_VER
#pragma warning(push)
#pragma warning (disable:4996)  // function or variable may be unsafe.
#endif //_MSC_VER


namespace Theron
{
namespace Detail
{


/**
Static class that manages a pool of unique strings.
*/
class StringPool
{
public:

    /**
    Holds a reference to the string pool, ensuring that it has been created.
    */
    class Ref
    {
    public:

        inline Ref()
        {
            StringPool::Reference();
        }

        inline ~Ref()
        {
            StringPool::Dereference();
        }
    };

    friend class Ref;

    /**
    Gets the address of the pooled version of the given literal string.
    The pooled version is created if it doesn't already exist.
    */
    inline static const char *Get(const char *const str);

private:

    class Entry : public List<Entry>::Node
    {
    public:

        inline explicit Entry(const char *const str) : mValue(0)
        {
            IAllocator *const allocator(AllocatorManager::Instance().GetAllocator());
            const uint32_t length(static_cast<uint32_t>(strlen(str)));
            const uint32_t roundedLength(THERON_ROUNDUP(length + 1, 4));

            mValue = reinterpret_cast<char *>(allocator->Allocate(roundedLength));

            strcpy(mValue, str);
        }

        inline ~Entry()
        {
            IAllocator *const allocator(AllocatorManager::Instance().GetAllocator());
            allocator->Free(mValue);
        }

        THERON_FORCEINLINE const char *Value() const
        {
            return mValue;
        }

    private:

        char *mValue;
    };

    class Container
    {
    public:

        Container();
        ~Container();

        const char *Get(const char *const str);

    private:

        Mutex mMutex;
        List<Entry> mEntries;
    };

    /**
    References the string pool, creating the singleton instance if it doesn't already exist.
    */
    static void Reference();

    /**
    Releases a references on the string pool, destroying the singleton instance if it is no longer referenced.
    */
    static void Dereference();

    static Container *smContainer;              ///< Pointer to the owned container instance.
    static Mutex smMutex;                       ///< Synchronization object protecting access.
    static uint32_t smReferenceCount;           ///< Counts the number of entities registered.
};


THERON_FORCEINLINE const char *StringPool::Get(const char *const str)
{
    THERON_ASSERT(smContainer);
    THERON_ASSERT(str);

    return smContainer->Get(str);
}


} // namespace Detail
} // namespace Theron


#ifdef _MSC_VER
#pragma warning(pop)
#endif //_MSC_VER


#endif // THERON_DETAIL_STRINGS_STRINGPOOL_H

