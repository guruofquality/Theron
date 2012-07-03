// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_TESTS_TESTSUITES_MESSAGECACHETESTSUITE_H
#define THERON_TESTS_TESTSUITES_MESSAGECACHETESTSUITE_H


#include <Theron/Align.h>
#include <Theron/BasicTypes.h>

#include <Theron/Detail/Allocators/CachingAllocator.h>

#include "TestFramework/TestSuite.h"


namespace UnitTests
{


/// A suite of unit tests.
class CachingAllocatorTestSuite : public TestFramework::TestSuite
{
public:

    /// Default constructor. Registers the tests.
    inline CachingAllocatorTestSuite()
    {
        TESTFRAMEWORK_REGISTER_TESTSUITE(CachingAllocatorTestSuite);

        TESTFRAMEWORK_REGISTER_TEST(TestConstruct);
        TESTFRAMEWORK_REGISTER_TEST(TestAllocateFree);
        TESTFRAMEWORK_REGISTER_TEST(TestAllocateAfterFree);
        TESTFRAMEWORK_REGISTER_TEST(TestAllocateAfterFreeSmaller);
        TESTFRAMEWORK_REGISTER_TEST(TestAllocateAfterFreeLarger);
        TESTFRAMEWORK_REGISTER_TEST(TestAllocateAfterFreeLargerAlignment);
        TESTFRAMEWORK_REGISTER_TEST(TestAllocateAfterFreeSmallerAlignment);
    }

    /// Virtual destructor
    inline virtual ~CachingAllocatorTestSuite()
    {
    }

    inline static void TestConstruct()
    {
        CacheType cache;
    }

    inline static void TestAllocateFree()
    {
        CacheType cache;

        void *const mem0(cache.AllocateAligned(sizeof(Item), THERON_ALIGNOF(Item)));
        Check(mem0 != 0, "Allocate failed");
        cache.Free(mem0, sizeof(Item));
    }

    inline static void TestAllocateAfterFree()
    {
        CacheType cache;

        void *const mem0(cache.AllocateAligned(sizeof(Item), THERON_ALIGNOF(Item)));
        Check(mem0 != 0, "Allocate failed");
        cache.Free(mem0, sizeof(Item));

        void *const mem1(cache.AllocateAligned(sizeof(Item), THERON_ALIGNOF(Item)));
        Check(mem1 != 0, "Allocate failed");
        cache.Free(mem1, sizeof(Item));

        Check(mem0 == mem1, "Second allocate didn't reuse free block");
    }

    inline static void TestAllocateAfterFreeSmaller()
    {
        CacheType cache;

        void *const mem0(cache.AllocateAligned(sizeof(Item) * 2, THERON_ALIGNOF(Item)));
        Check(mem0 != 0, "Allocate failed");
        cache.Free(mem0, sizeof(Item) * 2);

        void *const mem1(cache.AllocateAligned(sizeof(Item), THERON_ALIGNOF(Item)));
        Check(mem1 != 0, "Allocate failed");
        cache.Free(mem1, sizeof(Item));

        Check(mem0 != mem1, "Second allocate reuse free block of larger size");
    }

    inline static void TestAllocateAfterFreeLarger()
    {
        CacheType cache;

        void *const mem0(cache.AllocateAligned(sizeof(Item), THERON_ALIGNOF(Item)));
        Check(mem0 != 0, "Allocate failed");
        cache.Free(mem0, sizeof(Item));

        void *const mem1(cache.AllocateAligned(sizeof(Item) * 2, THERON_ALIGNOF(Item)));
        Check(mem1 != 0, "Allocate failed");
        cache.Free(mem1, sizeof(Item) * 2);

        Check(mem0 != mem1, "Second allocate reuse free block of larger size");
    }

    inline static void TestAllocateAfterFreeLargerAlignment()
    {
        CacheType cache;

        void *const mem0(cache.AllocateAligned(sizeof(Item), THERON_ALIGNOF(Item)));
        Check(mem0 != 0, "Allocate failed");
        cache.Free(mem0, sizeof(Item));

        void *const mem1(cache.AllocateAligned(sizeof(Item), THERON_ALIGNOF(Item) * 2));
        Check(mem1 != 0, "Allocate failed");
        cache.Free(mem1, sizeof(Item));

        // The first block may happen to be aligned anyway.
        if (THERON_ALIGNED(mem0, THERON_ALIGNOF(Item) * 2))
        {
            Check(mem0 == mem1, "Second allocate failed to reuse aligned free block");
        }
        else
        {
            Check(mem0 != mem1, "Second allocate reused non-aligned free block");
        }
    }

    inline static void TestAllocateAfterFreeSmallerAlignment()
    {
        CacheType cache;

        void *const mem0(cache.AllocateAligned(sizeof(Item), THERON_ALIGNOF(Item) * 2));
        Check(mem0 != 0, "Allocate failed");
        cache.Free(mem0, sizeof(Item));

        void *const mem1(cache.AllocateAligned(sizeof(Item), THERON_ALIGNOF(Item)));
        Check(mem1 != 0, "Allocate failed");
        cache.Free(mem1, sizeof(Item));

        Check(mem0 == mem1, "Second allocate failed to reuse aligned free block");
    }

private:

    typedef Theron::Detail::CachingAllocator<32> CacheType;

    struct Item
    {
        int a;
        int b;
    };
};


} // namespace UnitTests


#endif // THERON_TESTS_TESTSUITES_MESSAGECACHETESTSUITE_H

