// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


//
// This benchmarks measures the throughput with which messages can be queued and processed.
// Note that for large numbers of messages, this benchmark may be memory-limited.
//
// Create an actor that accepts two messages:
// * A positive integer that is added to an existing count.
// * A GetAndReset message that returns the current count and sets the count to 0.
//
// Send the actor n AddCount messages, followed by a GetAndReset message, and check the total.
//


#include <stdio.h>

// Enable checking for unregistered message types.
#define THERON_ENABLE_MESSAGE_REGISTRATION_CHECKS 1

#include <Theron/Actor.h>
#include <Theron/AllocatorManager.h>
#include <Theron/DefaultAllocator.h>
#include <Theron/Framework.h>
#include <Theron/Receiver.h>
#include <Theron/Register.h>

#include "../Common/Timer.h"


class Counter : public Theron::Actor
{
public:

    struct GetAndReset
    {
    };

    inline Counter() : mCount(0)
    {
        RegisterHandler(this, &Counter::HandleAdd);
        RegisterHandler(this, &Counter::HandleGetAndReset);
    }

private:

    inline void HandleAdd(const int &value, const Theron::Address /*from*/)
    {
        mCount += value;
    }

    inline void HandleGetAndReset(const GetAndReset &/*message*/, const Theron::Address from)
    {
        Send(mCount, from);
        mCount = 0;
    }

    int mCount;
};


// Register the message types so that registered names are used instead of dynamic_cast.
THERON_REGISTER_MESSAGE(int);
THERON_REGISTER_MESSAGE(Counter::GetAndReset);


struct CountCatcher
{
    inline void Catch(const int &value, const Theron::Address /*from*/) { mCount = value; }
    int mCount;
};


int main(int argc, char *argv[])
{
    int numMessagesProcessed(0), numThreadsPulsed(0), numThreadsWoken(0);
    CountCatcher countCatcher;

    const int numAdds = (argc > 1 && atoi(argv[1]) > 0) ? atoi(argv[1]) : 3000000;
    const int numThreads = (argc > 2 && atoi(argv[2]) > 0) ? atoi(argv[2]) : 16;
    const int increment = (argc > 3 && atoi(argv[3]) > 0) ? atoi(argv[3]) : 1;

    printf("Using numAdds = %d (use first command line argument to change)\n", numAdds);
    printf("Using numThreads = %d (use second command line argument to change)\n", numThreads);
    printf("Using increment = %d (use third command line argument to change)\n", increment);
    printf("Processing...\n");

    // The reported time includes the startup and cleanup cost.
    Timer timer;
    timer.Start();

    {
        Theron::Framework framework(numThreads);
        Theron::ActorRef counter(framework.CreateActor<Counter>());
        Theron::Receiver receiver;
        receiver.RegisterHandler(&countCatcher, &CountCatcher::Catch);

        // Add the increment to the counter n times.
        for (int i = 0; i < numAdds; ++i)
        {
            framework.Send(increment, receiver.GetAddress(), counter.GetAddress());
        }

        // Get the counter value.
        framework.Send(Counter::GetAndReset(), receiver.GetAddress(), counter.GetAddress());
        receiver.Wait();

        numMessagesProcessed = framework.GetCounterValue(Theron::Framework::COUNTER_MESSAGES_PROCESSED);
        numThreadsPulsed = framework.GetCounterValue(Theron::Framework::COUNTER_THREADS_PULSED);
        numThreadsWoken = framework.GetCounterValue(Theron::Framework::COUNTER_THREADS_WOKEN);
    }

    timer.Stop();
    printf("Processed %d messages in %.1f seconds, final count = %d\n", numMessagesProcessed, timer.Seconds(), countCatcher.mCount);
    printf("Threads pulsed: %d, woken: %d\n", numThreadsPulsed, numThreadsWoken);

#if THERON_ENABLE_DEFAULTALLOCATOR_CHECKS
    Theron::IAllocator *const allocator(Theron::AllocatorManager::Instance().GetAllocator());
    const int peakBytesAllocated(static_cast<Theron::DefaultAllocator *>(allocator)->GetPeakBytesAllocated());
    printf("Peak memory usage in bytes: %d bytes\n", peakBytesAllocated);
#endif // THERON_ENABLE_DEFAULTALLOCATOR_CHECKS

}

