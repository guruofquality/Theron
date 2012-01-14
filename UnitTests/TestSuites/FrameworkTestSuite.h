// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_TESTS_TESTSUITES_FRAMEWORKTESTSUITE_H
#define THERON_TESTS_TESTSUITES_FRAMEWORKTESTSUITE_H


#include <Theron/Framework.h>

#include "TestFramework/TestSuite.h"


namespace UnitTests
{


class FrameworkTestSuite : public TestFramework::TestSuite
{
public:

    class SimpleActor : public Theron::Actor
    {
    public:

        struct Parameters
        {
        };

        inline SimpleActor()
        {
        }

        inline explicit SimpleActor(const Parameters &/*params*/)
        {
        }
    };

    class ResponderActor : public Theron::Actor
    {
    public:

        inline ResponderActor()
        {
            RegisterHandler(this, &ResponderActor::Handler);
        }

    private:

        inline void Handler(const Theron::uint32_t &value, const Theron::Address from)
        {
            Send(value, from);
        }
    };

    class ThreadCountActor : public Theron::Actor
    {
    public:

        inline ThreadCountActor()
        {
            RegisterHandler(this, &ThreadCountActor::SetNumThreads);
        }

    private:

        inline void SetNumThreads(const int &numThreads, const Theron::Address from)
        {
            GetFramework().SetMinThreads(numThreads);
            GetFramework().SetMaxThreads(numThreads);

            // Send the message back to synchronize termination.
            Send(numThreads, from);
        }
    };

    inline FrameworkTestSuite()
    {
        TESTFRAMEWORK_REGISTER_TESTSUITE(FrameworkTestSuite);
    
        TESTFRAMEWORK_REGISTER_TEST(TestDefaultConstruction);
        TESTFRAMEWORK_REGISTER_TEST(TestExplicitConstruction);
        TESTFRAMEWORK_REGISTER_TEST(TestCreateActorNoParams);
        TESTFRAMEWORK_REGISTER_TEST(TestCreateActorWithParams);
        TESTFRAMEWORK_REGISTER_TEST(TestGetNumThreadsInitial);
        TESTFRAMEWORK_REGISTER_TEST(TestGetNumThreadsAfterSetMinThreads);
        TESTFRAMEWORK_REGISTER_TEST(TestGetNumThreadsAfterSetMaxThreads);
        TESTFRAMEWORK_REGISTER_TEST(TestGetPeakThreadsInitial);
        TESTFRAMEWORK_REGISTER_TEST(TestGetPeakThreadsAfterSetMinThreads);
        TESTFRAMEWORK_REGISTER_TEST(TestGetPeakThreadsAfterSetMaxThreads);
        TESTFRAMEWORK_REGISTER_TEST(TestGetMaxThreadsDefault);
        TESTFRAMEWORK_REGISTER_TEST(TestGetMaxThreadsInitial);
        TESTFRAMEWORK_REGISTER_TEST(TestGetMaxThreadsAfterSetMaxThreads);
        TESTFRAMEWORK_REGISTER_TEST(TestGetMaxThreadsAfterSetMinThreads);
        TESTFRAMEWORK_REGISTER_TEST(TestGetMinThreadsDefault);
        TESTFRAMEWORK_REGISTER_TEST(TestGetMinThreadsInitial);
        TESTFRAMEWORK_REGISTER_TEST(TestGetMinThreadsAfterSetMinThreads);
        TESTFRAMEWORK_REGISTER_TEST(TestGetMinThreadsAfterSetMaxThreads);
        TESTFRAMEWORK_REGISTER_TEST(TestGetNumMessagesProcessed);
        TESTFRAMEWORK_REGISTER_TEST(TestGetNumThreadPulses);
        TESTFRAMEWORK_REGISTER_TEST(TestGetNumWokenThreadsSerial);
        TESTFRAMEWORK_REGISTER_TEST(TestGetNumWokenThreadsParallel);
        TESTFRAMEWORK_REGISTER_TEST(TestResetCounters);
        TESTFRAMEWORK_REGISTER_TEST(TestThreadPoolThreadsafety);
    }

    inline virtual ~FrameworkTestSuite()
    {
    }

    inline static void TestDefaultConstruction()
    {
        Theron::Framework framework;
    }

    inline static void TestExplicitConstruction()
    {
        Theron::Framework framework(2);
    }

    inline static void TestCreateActorNoParams()
    {
        Theron::Framework framework;
        Theron::ActorRef actorRef(framework.CreateActor<SimpleActor>());
    }

    inline static void TestCreateActorWithParams()
    {
        Theron::Framework framework;
        SimpleActor::Parameters params;
        Theron::ActorRef actorRef(framework.CreateActor<SimpleActor>(params));
    }

    inline static void TestGetNumThreadsInitial()
    {
        Theron::Framework framework(1);
        Check(framework.GetNumThreads() == 1, "Too many threads created");
    }

    inline static void TestGetNumThreadsAfterSetMinThreads()
    {
        Theron::Framework framework(1);
        framework.SetMinThreads(3);

        // Thread count should eventually reach the requested limit.
        // But we don't want to busy-wait in the test so we just check the bound.
        Check(framework.GetNumThreads() >= 1, "Too few threads");
    }

    inline static void TestGetNumThreadsAfterSetMaxThreads()
    {
        Theron::Framework framework(3);
        Theron::Receiver receiver;
        Theron::ActorRef actorRef(framework.CreateActor<SimpleActor>());

        // Thread count should eventually reach the requested limit.
        // But we don't want to busy-wait in the test so we just check the bound.
        Check(framework.GetNumThreads() >= 1, "Too few threads");

        framework.SetMaxThreads(1);
        Check(framework.GetNumThreads() <= 3, "Too many threads");
    }

    inline static void TestGetPeakThreadsInitial()
    {
        Theron::Framework framework(3);

        // Thread count should eventually reach the requested limit.
        // But we don't want to busy-wait in the test so we just check the bound.
        Check(framework.GetPeakThreads() <= 3, "Too many threads created");
    }

    inline static void TestGetPeakThreadsAfterSetMinThreads()
    {
        Theron::Framework framework(1);
        framework.SetMinThreads(3);

        // Thread count should eventually reach the requested limit.
        // But we don't want to busy-wait in the test so we just check the bound.
        Check(framework.GetPeakThreads() >= 1, "Too few threads");
    }

    inline static void TestGetPeakThreadsAfterSetMaxThreads()
    {
        Theron::Framework framework(3);
        Theron::Receiver receiver;
        Theron::ActorRef actorRef(framework.CreateActor<SimpleActor>());

        // Thread count should eventually reach the requested limit.
        // But we don't want to busy-wait in the test so we just check the bound.
        Check(framework.GetPeakThreads() <= 3, "Peak thread count incorrect");

        framework.SetMaxThreads(1);
        Check(framework.GetPeakThreads() <= 3, "Peak thread count incorrect");
    }

    inline static void TestGetMaxThreadsDefault()
    {
        Theron::Framework framework;
        Check(framework.GetMaxThreads() >= 2, "Bad default max thread limit");
    }

    inline static void TestGetMaxThreadsInitial()
    {
        Theron::Framework framework(5);
        Check(framework.GetMaxThreads() >= 5, "Bad initial max thread limit");
    }

    inline static void TestGetMaxThreadsAfterSetMaxThreads()
    {
        // Setting a higher maximum may have no effect but shouldn't reduce it.
        Theron::Framework framework(2);
        framework.SetMaxThreads(5);
        Check(framework.GetMaxThreads() >= 2, "Bad max thread limit");
        Check(framework.GetMaxThreads() <= 5, "Bad max thread limit");
    }

    inline static void TestGetMaxThreadsAfterSetMinThreads()
    {
        Theron::Framework framework(5);
        
        // Setting a lower minimum may or may not cause the maximum to drop.
        framework.SetMinThreads(2);
        Check(framework.GetMaxThreads() >= 2, "Max thread limit too low");
        Check(framework.GetMaxThreads() <= 5, "Max thread limit too high");

        // Setting a higher minimum should cause the maximum to rise.
        framework.SetMinThreads(7);
        Check(framework.GetMaxThreads() >= 7, "Max thread limit too low");
    }

    inline static void TestGetMinThreadsDefault()
    {
        Theron::Framework framework;
        Check(framework.GetMinThreads() <= 2, "Bad default min thread limit");
    }

    inline static void TestGetMinThreadsInitial()
    {
        Theron::Framework framework(5);
        Check(framework.GetMinThreads() <= 5, "Bad initial min thread limit");
    }

    inline static void TestGetMinThreadsAfterSetMinThreads()
    {
        // Setting a lower minimum may have no effect but shouldn't reduce it.
        Theron::Framework framework(5);
        framework.SetMinThreads(2);
        Check(framework.GetMinThreads() >= 2, "Bad min thread limit");
        Check(framework.GetMinThreads() <= 5, "Bad min thread limit");
    }

    inline static void TestGetMinThreadsAfterSetMaxThreads()
    {
        Theron::Framework framework(3);
        
        // Setting a higher maximum may or may not cause the maximum to rise.
        framework.SetMaxThreads(5);
        Check(framework.GetMinThreads() <= 5, "Min thread limit too high");

        // Setting a lower maximum should cause the minimum to drop.
        framework.SetMaxThreads(2);
        Check(framework.GetMinThreads() <= 2, "Min thread limit too high");
    }

    inline static void TestGetNumMessagesProcessed()
    {
        Theron::Framework framework(2);
        Theron::Receiver receiver;

        // Local scope to check that actor destruction isn't counted as message processing.
        {
            // Create two responders that simply return integers sent to them.
            // We want two in parallel to check thread-safety.
            Theron::ActorRef actorOne(framework.CreateActor<ResponderActor>());
            Theron::ActorRef actorTwo(framework.CreateActor<ResponderActor>());

            // Send n messages to each responder.
            for (int count = 0; count < 100; ++count)
            {
                framework.Send(static_cast<Theron::uint32_t>(count), receiver.GetAddress(), actorOne.GetAddress());
                framework.Send(static_cast<Theron::uint32_t>(count), receiver.GetAddress(), actorTwo.GetAddress());
            }

            // Wait for all the replies.
            for (int count = 0; count < 100; ++count)
            {
                receiver.Wait();
                receiver.Wait();
            }
        }

        Check(framework.GetCounterValue(Theron::Framework::COUNTER_MESSAGES_PROCESSED) == 200, "Processed message count incorrect");
    }

    inline static void TestGetNumThreadPulses()
    {
        Theron::Framework framework(2);
        Theron::Receiver receiver;

        {
            // Create a responder that simply returns integers sent to it.
            Theron::ActorRef actor(framework.CreateActor<ResponderActor>());

            // Send n messages to each responder.
            for (int count = 0; count < 100; ++count)
            {
                framework.Send(static_cast<Theron::uint32_t>(count), receiver.GetAddress(), actor.GetAddress());
            }

            // Wait for all the replies.
            for (int count = 0; count < 100; ++count)
            {
                receiver.Wait();
            }
        }

        // We expect many of the messages to arrive while the actor is being processed.
        // Such messages shouldn't cause the threadpool to be pulsed, so aren't counted.
        // But it's non-deterministic, so we can't say much.
        Check(framework.GetCounterValue(Theron::Framework::COUNTER_THREADS_PULSED) <= 100, "Processed message count incorrect");
    }

    inline static void TestGetNumWokenThreadsSerial()
    {
        Theron::Framework framework(2);
        Theron::Receiver receiver;

        {
            // Create a responder that simply returns integers sent to it.
            Theron::ActorRef actor(framework.CreateActor<ResponderActor>());

            // Send n messages to each responder.
            for (int count = 0; count < 100; ++count)
            {
                framework.Send(static_cast<Theron::uint32_t>(count), receiver.GetAddress(), actor.GetAddress());
            }

            // Wait for all the replies.
            for (int count = 0; count < 100; ++count)
            {
                receiver.Wait();
            }
        }

        // We expect many of the messages to arrive while the actor is being processed.
        // Such messages shouldn't cause the threadpool to be pulsed, so don't wake any threads.
        // But it's non-deterministic, so we can't say much.
        Check(framework.GetCounterValue(Theron::Framework::COUNTER_THREADS_WOKEN) <= 100, "Woken thread count incorrect");
    }

    inline static void TestGetNumWokenThreadsParallel()
    {
        Theron::Framework framework(5);
        Theron::Receiver receiver;

        {
            // Create 5 responders that simply return integers sent to them.
            // We need them all to active in parallel to ensure the threads are woken.
            Theron::ActorRef actors[5] =
            {
                framework.CreateActor<ResponderActor>(),
                framework.CreateActor<ResponderActor>(),
                framework.CreateActor<ResponderActor>(),
                framework.CreateActor<ResponderActor>(),
                framework.CreateActor<ResponderActor>()
            };

            // Send n messages to each responder.
            for (int count = 0; count < 100; ++count)
            {
                framework.Send(static_cast<Theron::uint32_t>(count), receiver.GetAddress(), actors[0].GetAddress());
                framework.Send(static_cast<Theron::uint32_t>(count), receiver.GetAddress(), actors[1].GetAddress());
                framework.Send(static_cast<Theron::uint32_t>(count), receiver.GetAddress(), actors[2].GetAddress());
                framework.Send(static_cast<Theron::uint32_t>(count), receiver.GetAddress(), actors[3].GetAddress());
                framework.Send(static_cast<Theron::uint32_t>(count), receiver.GetAddress(), actors[4].GetAddress());
            }

            // Wait for all the replies.
            for (int count = 0; count < 100; ++count)
            {
                receiver.Wait();
                receiver.Wait();
                receiver.Wait();
                receiver.Wait();
                receiver.Wait();
            }
        }

        // We expect many of the messages to arrive while other actors are being processed, keeping the threads busy.
        // Such messages should cause the threadpool to be pulsed, waking the other threads.
        // Due to the non-deterministic nature it's possible that some threads go to sleep and are woken again.
        // It's also possible that some or all of the threads never slept at all so never needed to be woken.
        // Therefore it's not possible to test much here.
        Check(framework.GetCounterValue(Theron::Framework::COUNTER_THREADS_WOKEN) <= 500, "Woken thread count incorrect");
    }

    inline static void TestResetCounters()
    {
        Theron::Framework framework(2);
        Theron::Receiver receiver;

        {
            // Create two responders that simply return integers sent to them.
            // We want two in parallel to check thread-safety.
            Theron::ActorRef actorOne(framework.CreateActor<ResponderActor>());
            Theron::ActorRef actorTwo(framework.CreateActor<ResponderActor>());

            // Send n messages to each responder.
            for (int count = 0; count < 100; ++count)
            {
                framework.Send(static_cast<Theron::uint32_t>(count), receiver.GetAddress(), actorOne.GetAddress());
                framework.Send(static_cast<Theron::uint32_t>(count), receiver.GetAddress(), actorTwo.GetAddress());
            }

            // Wait for all the replies.
            for (int count = 0; count < 100; ++count)
            {
                receiver.Wait();
                receiver.Wait();
            }
        }

        // Reset the counters.
        framework.ResetCounters();

        // The worker threads may still be woken to garbage collect the actors.
        Check(framework.GetCounterValue(Theron::Framework::COUNTER_MESSAGES_PROCESSED) == 0, "Message processing count not reset");
        Check(framework.GetCounterValue(Theron::Framework::COUNTER_THREADS_PULSED) == 0, "Thread pulse count not reset");
        Check(framework.GetCounterValue(Theron::Framework::COUNTER_THREADS_WOKEN) <= 2, "Woken thread count not reset");
    }

    inline static void TestThreadPoolThreadsafety()
    {
        Theron::Framework framework;
        Theron::Receiver receiver;

        // Create two actors that each set the number of framework threads in response to messages.
        Theron::ActorRef actorOne(framework.CreateActor<ThreadCountActor>());
        Theron::ActorRef actorTwo(framework.CreateActor<ThreadCountActor>());

        // Send a large number of messages, causing both actors to swamp the shared framework
        // with requests to change the number of worker threads.
        for (int count = 0; count < 100; ++count)
        {
            framework.Send(5, receiver.GetAddress(), actorOne.GetAddress());
            framework.Send(10, receiver.GetAddress(), actorTwo.GetAddress());
            framework.Send(12, receiver.GetAddress(), actorOne.GetAddress());
            framework.Send(7, receiver.GetAddress(), actorTwo.GetAddress());
        }

        // Wait for all replies before terminating.
        for (int count = 0; count < 100; ++count)
        {
            receiver.Wait();
            receiver.Wait();
            receiver.Wait();
            receiver.Wait();
        }
    }
};


} // namespace UnitTests


#endif // THERON_TESTS_TESTSUITES_FRAMEWORKTESTSUITE_H

