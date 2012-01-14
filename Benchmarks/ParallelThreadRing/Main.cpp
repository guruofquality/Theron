// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


//
// This benchmark implements a variant on the "thread-ring" benchmark intended to
// measure the raw speed of message passing. In the traditional benchmark, a single
// "token" message is sent around a ring of connected actors. The token is an integer
// counter, and the counter is decremented with every hop. When the token value reaches
// zero the identity of the actor holding the token is printed out.
//
// Note that the processing within each actor is trivial and consists only of decrementing
// the value of the token it received in a message, and forwarding it to the next actor
// in the ring in another message. To form the ring, each actor is provided with the address
// of the next actor in the ring on construction.
//
// Unlike the standard benchmark, this variant creates 503 tokens instead of just one.
// Each of the actors in the ring is passed one token initially. As before, the tokens
// are passed around the ring and decremented with each hop until they reach zero. The
// waiting main program terminates when it has received all of the replies, indicating
// all the tokens have reached zero. The total number of "hops" to be performed is split
// equally between the 503 tokens, so that between them they perform the intended number
// of hops, but in parallel.
//
// This benchmark is a more severe (and arguably more meaningful) measure of the raw
// performance of an Actor Model implementation, since it involves parallel message
// passing and so contention over the shared components of the message passing mechanism.
// It's execution times show a lot of variability, presumably because the execution order
// (the order in which the actors are processed) is non-deterministic and can vary from
// one run to the next.
//


#include <stdio.h>

// Enable checking for unregistered message types.
#define THERON_ENABLE_MESSAGE_REGISTRATION_CHECKS 1

#include <Theron/Actor.h>
#include <Theron/Address.h>
#include <Theron/AllocatorManager.h>
#include <Theron/DefaultAllocator.h>
#include <Theron/Framework.h>
#include <Theron/Receiver.h>
#include <Theron/Register.h>

#include "../Common/Timer.h"


static const int NUM_ACTORS = 503;
static const int NUM_TOKENS = 503;


class Member : public Theron::Actor
{
public:

    inline Member()
    {
        RegisterHandler(this, &Member::InitHandler);
    }

private:

    inline void InitHandler(const Theron::Address &next, const Theron::Address from)
    {
        mNext = next;
        mCaller = from;

        RegisterHandler(this, &Member::TokenHandler);
        DeregisterHandler(this, &Member::InitHandler);
    }

    inline void TokenHandler(const int &token, const Theron::Address /*from*/)
    {
        if (token > 0)
        {
            TailSend(token - 1, mNext);
            return;
        }

        TailSend(token, mCaller);
    }

    Theron::Address mNext;
    Theron::Address mCaller;
};


// Register the message types so that registered names are used instead of dynamic_cast.
THERON_REGISTER_MESSAGE(int);
THERON_REGISTER_MESSAGE(Theron::Address);


int main(int argc, char *argv[])
{
    int numMessagesProcessed(0), numThreadsPulsed(0), numThreadsWoken(0);
    const int numHops = (argc > 1 && atoi(argv[1]) > 0) ? atoi(argv[1]) : 50000000;
    const int numThreads = (argc > 2 && atoi(argv[2]) > 0) ? atoi(argv[2]) : 16;
    const int hopsPerToken((numHops + NUM_TOKENS - 1) / NUM_TOKENS);

    printf("Using numHops = %d (use first command line argument to change)\n", numHops);
    printf("Using numThreads = %d (use second command line argument to change)\n", numThreads);
    printf("Starting %d tokens in a ring of %d actors...\n", NUM_TOKENS, NUM_ACTORS);

    // The reported time includes the startup and cleanup cost.
    Timer timer;
    timer.Start();

    {
        Theron::Framework framework(numThreads);
        Theron::ActorRef members[NUM_ACTORS];
        Theron::Receiver receiver;

        // Create the member actors.
        for (int index = 0; index < NUM_ACTORS; ++index)
        {
            members[index] = framework.CreateActor<Member>();
        }

        // Initialize the actors by passing each one the address of the next actor in the ring.
        for (int index(NUM_ACTORS - 1), nextIndex(0); index >= 0; nextIndex = index--)
        {
            framework.Send(members[nextIndex].GetAddress(), receiver.GetAddress(), members[index].GetAddress());
        }

        // Start the processing by sending a token each to a number of consecutive actors.
        // We divide the total number of hops between the tokens so they're equally long-lived.
        for (int index(0), hopsLeft(numHops); index < NUM_TOKENS; ++index)
        {
            const int hopsForThisToken(hopsLeft < hopsPerToken ? hopsLeft : hopsPerToken);
            framework.Send(hopsForThisToken, receiver.GetAddress(), members[index].GetAddress());
            hopsLeft -= hopsForThisToken;
        }

        // Wait for all signal messages, indicating the tokens have all reached zero.
        int outstandingCount(NUM_TOKENS);
        while (outstandingCount)
        {
            outstandingCount -= receiver.Wait(outstandingCount);
        }

        numMessagesProcessed = framework.GetCounterValue(Theron::Framework::COUNTER_MESSAGES_PROCESSED);
        numThreadsPulsed = framework.GetCounterValue(Theron::Framework::COUNTER_THREADS_PULSED);
        numThreadsWoken = framework.GetCounterValue(Theron::Framework::COUNTER_THREADS_WOKEN);
    }

    timer.Stop();
    printf("Processed %d messages in %.1f seconds\n", numMessagesProcessed, timer.Seconds());
    printf("Threads pulsed: %d, woken: %d\n", numThreadsPulsed, numThreadsWoken);

#if THERON_ENABLE_DEFAULTALLOCATOR_CHECKS
    Theron::IAllocator *const allocator(Theron::AllocatorManager::Instance().GetAllocator());
    const int peakBytesAllocated(static_cast<Theron::DefaultAllocator *>(allocator)->GetPeakBytesAllocated());
    printf("Peak memory usage in bytes: %d bytes\n", peakBytesAllocated);
#endif // THERON_ENABLE_DEFAULTALLOCATOR_CHECKS

}

