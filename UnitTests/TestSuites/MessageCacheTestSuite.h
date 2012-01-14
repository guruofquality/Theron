// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_TESTS_TESTSUITES_MESSAGECACHETESTSUITE_H
#define THERON_TESTS_TESTSUITES_MESSAGECACHETESTSUITE_H


#include <Theron/Detail/BasicTypes.h>
#include <Theron/Detail/MessageCache/MessageCache.h>
#include <Theron/Align.h>

#include "TestFramework/TestSuite.h"


namespace UnitTests
{


/// A suite of unit tests.
class MessageCacheTestSuite : public TestFramework::TestSuite
{
public:

    /// Default constructor. Registers the tests.
    inline MessageCacheTestSuite()
    {
        TESTFRAMEWORK_REGISTER_TESTSUITE(MessageCacheTestSuite);

        TESTFRAMEWORK_REGISTER_TEST(TestConstruct);
        TESTFRAMEWORK_REGISTER_TEST(TestInstance);
        TESTFRAMEWORK_REGISTER_TEST(TestAllocateFree);
        TESTFRAMEWORK_REGISTER_TEST(TestAllocateAfterFree);
        TESTFRAMEWORK_REGISTER_TEST(TestAllocateAfterFreeSmaller);
        TESTFRAMEWORK_REGISTER_TEST(TestAllocateAfterFreeLarger);
        TESTFRAMEWORK_REGISTER_TEST(TestAllocateAfterFreeLargerAlignment);
        TESTFRAMEWORK_REGISTER_TEST(TestAllocateAfterFreeSmallerAlignment);
    }

    /// Virtual destructor
    inline virtual ~MessageCacheTestSuite()
    {
    }

    inline static void TestConstruct()
    {
        Theron::Detail::MessageCache::Instance().Reference();
        Theron::Detail::MessageCache::Instance().Dereference();
    }

    inline static void TestInstance()
    {
        Theron::Detail::MessageCache::Instance().Reference();
        Theron::Detail::MessageCache &freeList(Theron::Detail::MessageCache::Instance());

        Check(&freeList != 0, "Instance null");

        Theron::Detail::MessageCache::Instance().Dereference();
    }

    inline static void TestAllocateFree()
    {
        Theron::Detail::MessageCache::Instance().Reference();
        Theron::Detail::MessageCache &freeList(Theron::Detail::MessageCache::Instance());

        void *const mem0(freeList.Allocate(sizeof(Item), THERON_ALIGNOF(Item)));
        Check(mem0 != 0, "Allocate failed");
        freeList.Free(mem0, sizeof(Item));

        Theron::Detail::MessageCache::Instance().Dereference();
    }

    inline static void TestAllocateAfterFree()
    {
        Theron::Detail::MessageCache::Instance().Reference();
        Theron::Detail::MessageCache &freeList(Theron::Detail::MessageCache::Instance());

        void *const mem0(freeList.Allocate(sizeof(Item), THERON_ALIGNOF(Item)));
        Check(mem0 != 0, "Allocate failed");
        freeList.Free(mem0, sizeof(Item));

        void *const mem1(freeList.Allocate(sizeof(Item), THERON_ALIGNOF(Item)));
        Check(mem1 != 0, "Allocate failed");
        freeList.Free(mem1, sizeof(Item));

        Check(mem0 == mem1, "Second allocate didn't reuse free block");

        Theron::Detail::MessageCache::Instance().Dereference();
    }

    inline static void TestAllocateAfterFreeSmaller()
    {
        Theron::Detail::MessageCache::Instance().Reference();
        Theron::Detail::MessageCache &freeList(Theron::Detail::MessageCache::Instance());

        void *const mem0(freeList.Allocate(sizeof(Item) * 2, THERON_ALIGNOF(Item)));
        Check(mem0 != 0, "Allocate failed");
        freeList.Free(mem0, sizeof(Item) * 2);

        void *const mem1(freeList.Allocate(sizeof(Item), THERON_ALIGNOF(Item)));
        Check(mem1 != 0, "Allocate failed");
        freeList.Free(mem1, sizeof(Item));

        Check(mem0 != mem1, "Second allocate reuse free block of larger size");

        Theron::Detail::MessageCache::Instance().Dereference();
    }

    inline static void TestAllocateAfterFreeLarger()
    {
        Theron::Detail::MessageCache::Instance().Reference();
        Theron::Detail::MessageCache &freeList(Theron::Detail::MessageCache::Instance());

        void *const mem0(freeList.Allocate(sizeof(Item), THERON_ALIGNOF(Item)));
        Check(mem0 != 0, "Allocate failed");
        freeList.Free(mem0, sizeof(Item));

        void *const mem1(freeList.Allocate(sizeof(Item) * 2, THERON_ALIGNOF(Item)));
        Check(mem1 != 0, "Allocate failed");
        freeList.Free(mem1, sizeof(Item) * 2);

        Check(mem0 != mem1, "Second allocate reuse free block of larger size");

        Theron::Detail::MessageCache::Instance().Dereference();
    }

    inline static void TestAllocateAfterFreeLargerAlignment()
    {
        Theron::Detail::MessageCache::Instance().Reference();
        Theron::Detail::MessageCache &freeList(Theron::Detail::MessageCache::Instance());

        void *const mem0(freeList.Allocate(sizeof(Item), THERON_ALIGNOF(Item)));
        Check(mem0 != 0, "Allocate failed");
        freeList.Free(mem0, sizeof(Item));

        void *const mem1(freeList.Allocate(sizeof(Item), THERON_ALIGNOF(Item) * 2));
        Check(mem1 != 0, "Allocate failed");
        freeList.Free(mem1, sizeof(Item));

        // The first block may happen to be aligned anyway.
        if (THERON_ALIGNED(mem0, THERON_ALIGNOF(Item) * 2))
        {
            Check(mem0 == mem1, "Second allocate failed to reuse aligned free block");
        }
        else
        {
            Check(mem0 != mem1, "Second allocate reused non-aligned free block");
        }

        Theron::Detail::MessageCache::Instance().Dereference();
    }

    inline static void TestAllocateAfterFreeSmallerAlignment()
    {
        Theron::Detail::MessageCache::Instance().Reference();
        Theron::Detail::MessageCache &freeList(Theron::Detail::MessageCache::Instance());

        void *const mem0(freeList.Allocate(sizeof(Item), THERON_ALIGNOF(Item) * 2));
        Check(mem0 != 0, "Allocate failed");
        freeList.Free(mem0, sizeof(Item));

        void *const mem1(freeList.Allocate(sizeof(Item), THERON_ALIGNOF(Item)));
        Check(mem1 != 0, "Allocate failed");
        freeList.Free(mem1, sizeof(Item));

        Check(mem0 == mem1, "Second allocate failed to reuse aligned free block");

        Theron::Detail::MessageCache::Instance().Dereference();
    }

private:

    struct Item
    {
        int a;
        int b;
    };
};


} // namespace UnitTests


#endif // THERON_TESTS_TESTSUITES_MESSAGECACHETESTSUITE_H

