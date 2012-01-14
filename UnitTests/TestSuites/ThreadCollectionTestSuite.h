// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_TESTS_TESTSUITES_THREADCOLLECTIONTESTSUITE_H
#define THERON_TESTS_TESTSUITES_THREADCOLLECTIONTESTSUITE_H


#include <Theron/Detail/BasicTypes.h>
#include <Theron/Detail/Containers/List.h>
#include <Theron/Detail/ThreadPool/ThreadCollection.h>
#include <Theron/Detail/Threading/Lock.h>
#include <Theron/Detail/Threading/Mutex.h>

#include "TestFramework/TestSuite.h"


namespace UnitTests
{


/// A suite of unit tests.
class ThreadCollectionTestSuite : public TestFramework::TestSuite
{
public:

    /// Default constructor. Registers the tests.
    inline ThreadCollectionTestSuite()
    {
        TESTFRAMEWORK_REGISTER_TESTSUITE(ThreadCollectionTestSuite);

        TESTFRAMEWORK_REGISTER_TEST(TestConstruct);
        TESTFRAMEWORK_REGISTER_TEST(TestOneThread);
        TESTFRAMEWORK_REGISTER_TEST(TestTwoThreads);
        TESTFRAMEWORK_REGISTER_TEST(TestThreadReuse);
    }

    /// Virtual destructor
    inline virtual ~ThreadCollectionTestSuite()
    {
    }

    inline static void TestConstruct()
    {
        Theron::Detail::ThreadCollection threadCollection;
    }

    inline static void TestOneThread()
    {
        Theron::Detail::ThreadCollection threadCollection;

        sHitCount = 0;
        Context context(0);

        threadCollection.CreateThread(StaticEntryPoint, &context);
        threadCollection.DestroyThreads();

        Check(sHitCount == 1, "Thread function wasn't run");
        Check(context.mValue == 1, "Thread function wasn't run");
    }

    inline static void TestTwoThreads()
    {
        Theron::Detail::ThreadCollection threadCollection;

        sHitCount = 0;
        Context context0(0);
        Context context1(0);

        threadCollection.CreateThread(StaticEntryPoint, &context0);
        threadCollection.CreateThread(StaticEntryPoint, &context1);

        threadCollection.DestroyThreads();

        Check(sHitCount == 2, "Thread function wasn't run");
        Check(context0.mValue == 1, "Thread function wasn't run");
        Check(context1.mValue == 1, "Thread function wasn't run");
    }

    inline static void TestThreadReuse()
    {
        Theron::Detail::ThreadCollection threadCollection;
        const Theron::uint32_t numThreads = 128;

        sHitCount = 0;
        Context contexts[numThreads];

        // Create n threads, each referencing a different context.
        for (Theron::uint32_t index = 0; index < numThreads; ++index)
        {
            threadCollection.CreateThread(StaticEntryPoint, &contexts[index]);
        }

        // Destroy all the threads.
        threadCollection.DestroyThreads();

        Check(sHitCount == numThreads, "Thread function wasn't run");
        for (Theron::uint32_t index = 0; index < numThreads; ++index)
        {
            Check(contexts[index].mValue == 1, "Thread function wasn't run");
        }
    }

private:

    typedef Theron::Detail::List<Theron::Detail::Thread *> ThreadList;

    struct Context
    {
        inline Context() : mValue(0)
        {
        }

        inline Context(const Theron::uint32_t value) : mValue(value)
        {
        }

        Theron::uint32_t mValue;
    };

    inline static void StaticEntryPoint(void *const context)
    {
        Context *const threadContext(reinterpret_cast<Context *>(context));
        if (threadContext)
        {
            // This is per-thread so doesn't need locking.
            ++threadContext->mValue;
        }

        {
            Theron::Detail::Lock lock(sMutex);
            ++sHitCount;
        }
    }

    static Theron::Detail::Mutex sMutex;
    static Theron::uint32_t sHitCount;
};


Theron::Detail::Mutex ThreadCollectionTestSuite::sMutex;
Theron::uint32_t ThreadCollectionTestSuite::sHitCount = 0;


} // namespace UnitTests


#endif // THERON_TESTS_TESTSUITES_THREADCOLLECTIONTESTSUITE_H

