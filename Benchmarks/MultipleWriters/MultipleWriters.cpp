// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include <Theron/Theron.h>

#include "../Common/Timer.h"


class Writer : public Theron::Actor
{
public:

    inline Writer(Theron::Framework &framework, const Theron::Address &reader) : Theron::Actor(framework), mReader(reader)
    {
        RegisterHandler(this, &Writer::TokenHandler);
    }

private:

    inline void TokenHandler(const int &token, const Theron::Address /*from*/)
    {
        // Forward the token to the reader.
        Send(token, mReader);
    }

    Theron::Address mReader;
};


class Reader : public Theron::Actor
{
public:

    inline Reader(Theron::Framework &framework, const Theron::Address &sink) : Theron::Actor(framework), mSink(sink)
    {
        RegisterHandler(this, &Reader::TokenHandler);
    }

private:

    inline void TokenHandler(const int &token, const Theron::Address /*from*/)
    {
        if (token == 0)
        {
            // Signal the waiting sink that we received a zero token.
            Send(0, mSink);
        }
    }

    Theron::Address mSink;
};


// Register the message types so that registered names are used instead of dynamic_cast.
THERON_DECLARE_REGISTERED_MESSAGE(int);
THERON_DEFINE_REGISTERED_MESSAGE(int);


int main(int argc, char *argv[])
{
    Theron::uint32_t messageCounts[32];
    Theron::uint32_t yieldCounts[32];
    Theron::uint32_t localPushCounts[32];
    Theron::uint32_t sharedPushCounts[32];

    const int numTokens = (argc > 1 && atoi(argv[1]) > 0) ? atoi(argv[1]) : 50000000;
    const int numThreads = (argc > 2 && atoi(argv[2]) > 0) ? atoi(argv[2]) : 16;
    const int numWriters = (argc > 3 && atoi(argv[3]) > 0) ? atoi(argv[3]) : 2;

    printf("Using numTokens = %d (use first command line argument to change)\n", numTokens);
    printf("Using numThreads = %d (use second command line argument to change)\n", numThreads);
    printf("Using numWriters = %d (use third command line argument to change)\n", numWriters);
    printf("Starting %d writers sending %d messages between them...\n", numWriters, numTokens);

    // The reported time includes the startup and cleanup cost.
    Timer timer;
    timer.Start();

    {
        Theron::Framework framework(numThreads);
        std::vector<Writer *> writers(numWriters);
        Theron::Receiver receiver;

        // Create the reader.
        Reader reader(framework, receiver.GetAddress());

        // Create n writers.
        for (int index = 0; index < numWriters; ++index)
        {
            writers[index] = new Writer(framework, reader.GetAddress());
        }

        // Send each of the writers a token in round-robin until all tokens have been sent.
        int count(numTokens);
        int index(0);
        while (--count >= 0)
        {
            framework.Send(count, Theron::Address::Null(), writers[index++]->GetAddress());
            index = (index == numWriters) ? 0 : index;
        }

        // Wait for the signal message indicating the reader has received the zero token.
        receiver.Wait();

        // Destroy the writers.
        for (int index = 0; index < numWriters; ++index)
        {
            delete writers[index];
        }

        framework.GetPerThreadCounterValues(Theron::COUNTER_MESSAGES_PROCESSED, messageCounts, 32);
        framework.GetPerThreadCounterValues(Theron::COUNTER_YIELDS, yieldCounts, 32);
        framework.GetPerThreadCounterValues(Theron::COUNTER_LOCAL_PUSHES, localPushCounts, 32);
        framework.GetPerThreadCounterValues(Theron::COUNTER_SHARED_PUSHES, sharedPushCounts, 32);
    }

    timer.Stop();

    printf("Processed in %.1f seconds\n", timer.Seconds());
    
    printf("Message:");
    for (int index = 0; index <= numThreads; ++index)
    {
        printf("% 10d", messageCounts[index]);
    }

    printf("\n");
    printf("Yield:  ");
    for (int index = 0; index <= numThreads; ++index)
    {
        printf("% 10d", yieldCounts[index]);
    }

    printf("\n");
    printf("Local:  ");
    for (int index = 0; index <= numThreads; ++index)
    {
        printf("% 10d", localPushCounts[index]);
    }

    printf("\n");
    printf("Shared: ");
    for (int index = 0; index <= numThreads; ++index)
    {
        printf("% 10d", sharedPushCounts[index]);
    }

    printf("\n");

#if THERON_ENABLE_DEFAULTALLOCATOR_CHECKS
    Theron::IAllocator *const allocator(Theron::AllocatorManager::Instance().GetAllocator());
    const int allocationCount(static_cast<Theron::DefaultAllocator *>(allocator)->GetAllocationCount());
    const int peakBytesAllocated(static_cast<Theron::DefaultAllocator *>(allocator)->GetPeakBytesAllocated());
    printf("Total number of allocations: %d calls\n", allocationCount);
    printf("Peak memory usage in bytes: %d bytes\n", peakBytesAllocated);
#endif // THERON_ENABLE_DEFAULTALLOCATOR_CHECKS

}

