// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


/*
* The Computer Language Benchmarks Game
* http://shootout.alioth.debian.org/
*/

//
// This is a performance benchmark. It implements a simple benchmark commonly used to
// measure the raw speed of message passing. A single "token" message is sent around
// a ring of connected actors. The token is an integer counter, and the counter is decremented
// with every hop. When the token value reaches zero the identity of the actor holding the token
// is printed out. Here's the official description from
// http://shootout.alioth.debian.org/u64q/performance.php?test=threadring
//
// - create 503 linked threads (named 1 to 503)
// - thread 503 should be linked to thread 1, forming an unbroken ring
// - pass a token to thread 1
// - pass the token from thread to thread N times
// - print the name of the last thread (1 to 503) to take the token
//
// Note that the processing within each actor is trivial and consists only of decrementing
// the value of the token it received in a message, and forwarding it to the next actor
// in the ring in another message. To form the ring, each actor is provided with the address
// of the next actor in the ring on construction.
//


#include <stdio.h>
#include <stdlib.h>

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


class Member : public Theron::Actor
{
public:

    inline Member(Theron::Framework &framework) : Theron::Actor(framework)
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
        int mssg(token);
        Theron::Address to(mCaller);

        if (token > 0)
        {
            mssg = token - 1;
            to = mNext;
        }

        TailSend(mssg, to);
    }

    Theron::Address mNext;
    Theron::Address mCaller;
};


// Register the message types so that registered names are used instead of dynamic_cast.
THERON_REGISTER_MESSAGE(int);
THERON_REGISTER_MESSAGE(Theron::Address);


struct AddressCatcher
{
    inline void Catch(const int &/*message*/, const Theron::Address from) { mAddress = from; }
    Theron::Address mAddress;
};


int main(int argc, char *argv[])
{
    int numMessagesProcessed(0), numThreadsPulsed(0), numThreadsWoken(0);
    AddressCatcher catcher;

    const int numHops = (argc > 1 && atoi(argv[1]) > 0) ? atoi(argv[1]) : 50000000;
    const int numThreads = (argc > 2 && atoi(argv[2]) > 0) ? atoi(argv[2]) : 16;

    printf("Using numHops = %d (use first command line argument to change)\n", numHops);
    printf("Using numThreads = %d (use second command line argument to change)\n", numThreads);
    printf("Starting one token in a ring of %d actors...\n", NUM_ACTORS);

    // The reported time includes the startup and cleanup cost.
    Timer timer;
    timer.Start();

    {
        Theron::Framework framework(numThreads);
        Member *members[NUM_ACTORS];

        Theron::Receiver receiver;
        receiver.RegisterHandler(&catcher, &AddressCatcher::Catch);

        // Create NUM_ACTORS member actors for the ring.
        for (int index = 0; index < NUM_ACTORS; ++index)
        {
            members[index] = new Member(framework);
        }

        // Initialize the actors by passing each one the address of the next actor in the ring.
        for (int index(NUM_ACTORS - 1), nextIndex(0); index >= 0; nextIndex = index--)
        {
            framework.Send(members[nextIndex]->GetAddress(), receiver.GetAddress(), members[index]->GetAddress());
        }

        // Start the processing by sending the token to the first actor.
        framework.Send(numHops, receiver.GetAddress(), members[0]->GetAddress());

        // Wait for the signal message indicating the tokens has reached zero.
        receiver.Wait();

        // Destroy the member actors.
        for (int index = 0; index < NUM_ACTORS; ++index)
        {
            delete members[index];
        }

        //numMessagesProcessed = framework.GetCounterValue(Theron::Framework::COUNTER_MESSAGES_PROCESSED);
        //numThreadsPulsed = framework.GetCounterValue(Theron::Framework::COUNTER_THREADS_PULSED);
        //numThreadsWoken = framework.GetCounterValue(Theron::Framework::COUNTER_THREADS_WOKEN);
    }

    timer.Stop();

    printf("Processed %d messages in %.1f seconds\n", numMessagesProcessed, timer.Seconds());
    printf("Token stopped at entity '%d'\n", catcher.mAddress.AsInteger());
    printf("Threads pulsed: %d, woken: %d\n", numThreadsPulsed, numThreadsWoken);

#if THERON_ENABLE_DEFAULTALLOCATOR_CHECKS
    Theron::IAllocator *const allocator(Theron::AllocatorManager::Instance().GetAllocator());
    const int peakBytesAllocated(static_cast<Theron::DefaultAllocator *>(allocator)->GetPeakBytesAllocated());
    printf("Peak memory usage in bytes: %d bytes\n", peakBytesAllocated);
#endif // THERON_ENABLE_DEFAULTALLOCATOR_CHECKS

}

