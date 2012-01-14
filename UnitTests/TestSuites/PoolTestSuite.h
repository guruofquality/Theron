// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_TESTS_TESTSUITES_POOLTESTSUITE_H
#define THERON_TESTS_TESTSUITES_POOLTESTSUITE_H


#include <Theron/Detail/BasicTypes.h>
#include <Theron/Detail/MessageCache/Pool.h>
#include <Theron/Align.h>

#include "TestFramework/TestSuite.h"


namespace UnitTests
{


/// A suite of unit tests.
class PoolTestSuite : public TestFramework::TestSuite
{
public:

    /// Default constructor. Registers the tests.
    inline PoolTestSuite()
    {
        TESTFRAMEWORK_REGISTER_TESTSUITE(PoolTestSuite);

        TESTFRAMEWORK_REGISTER_TEST(TestConstruct);
        TESTFRAMEWORK_REGISTER_TEST(TestAdd);
        TESTFRAMEWORK_REGISTER_TEST(TestEmptyAfterAdd);
        TESTFRAMEWORK_REGISTER_TEST(TestFetchWhileEmpty);
        TESTFRAMEWORK_REGISTER_TEST(TestFetchAfterAdd);
        TESTFRAMEWORK_REGISTER_TEST(TestEmptyAfterFetch);
        TESTFRAMEWORK_REGISTER_TEST(TestFetchSizeWhileEmpty);
        TESTFRAMEWORK_REGISTER_TEST(TestFetchCorrectAlignment);
        TESTFRAMEWORK_REGISTER_TEST(TestFetchWrongAlignment);
    }

    /// Virtual destructor
    inline virtual ~PoolTestSuite()
    {
    }

    inline static void TestConstruct()
    {
        Theron::Detail::Pool pool;
    }

    inline static void TestAdd()
    {
        Theron::Detail::Pool pool;

        Item item0;
        pool.Add(&item0);
    }

    inline static void TestEmptyAfterAdd()
    {
        Theron::Detail::Pool pool;

        Item item0;
        pool.Add(&item0);

        Check(pool.FetchAligned(THERON_ALIGNOF(Item)) == &item0, "Pool empty");
    }

    inline static void TestFetchWhileEmpty()
    {
        Theron::Detail::Pool pool;
        Check(pool.Fetch() == 0, "Fetch should fail when empty");
    }

    inline static void TestFetchAfterAdd()
    {
        Theron::Detail::Pool pool;

        Item item0;
        pool.Add(&item0);

        Check(pool.Fetch() == &item0, "Fetch failed");
    }

    inline static void TestEmptyAfterFetch()
    {
        Theron::Detail::Pool pool;

        Item item0;
        pool.Add(&item0);

        Check(pool.Fetch() == &item0, "Fetch failed");
        Check(pool.Fetch() == 0, "Pool should be empty after fetch");
    }

    inline static void TestFetchSizeWhileEmpty()
    {
        Theron::Detail::Pool pool;
        Check(pool.FetchAligned(THERON_ALIGNOF(Item)) == 0, "Fetch should fail when empty");
    }

    inline static void TestFetchCorrectAlignment()
    {
        Theron::Detail::Pool pool;

        Item item0;
        pool.Add(&item0);

        Check(pool.FetchAligned(THERON_ALIGNOF(Item)) == &item0, "Fetch failed");
        Check(pool.Fetch() == 0, "Pool should be empty after fetch");
    }

    inline static void TestFetchWrongAlignment()
    {
        Theron::Detail::Pool pool;

        Item item0;
        pool.Add(&item0);

        // The address may happen to be aligned.
        const Theron::uint32_t increasedAlignment(THERON_ALIGNOF(Item) * 2);
        if (THERON_ALIGNED(&item0, increasedAlignment))
        {
            Check(pool.FetchAligned(increasedAlignment) == &item0, "Fetch failed");
            Check(pool.Fetch() == 0, "Fetch shouldn't return anything");
        }
        else
        {
            Check(pool.FetchAligned(increasedAlignment) == 0, "Fetch should fail");
            Check(pool.Fetch() == &item0, "Fetch should return original item");
        }
    }

private:

    struct Item
    {
        int a;
        int b;
    };
};


} // namespace UnitTests


#endif // THERON_TESTS_TESTSUITES_POOLTESTSUITE_H

