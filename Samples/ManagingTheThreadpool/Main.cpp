// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


//
// This sample shows how to actively manage the size of a framework's threadpool.
// The main thread creates a number of Processor actors, each of which does some
// work in response to a received message, and then sends a message back.
// The main thread sits in a main program loop, sending more work messages to the
// Processors and collecting their results, until all results have been received.
// In addition the main thread creates a Manager actor, which is specialized for
// managing the threads of the Framework instance's threadpool. On receiving a
// message, the Manager queries the Framework for metrics recording the number
// of times a 'pulse' event tried to wake a thread in the threadpool in response
// to a message arriving at an actor, and how many times a thread was actually
// woken. By comparing these two counters the Manager determines, roughly, whether
// adding more threads to the threadpool would allow more actors to be run in
// parallel (in software threads, timeslices across the fixed number of hardware
// threads, so increasing parallelism at the expense of thread-switching overhead).
// The Manager then acts to increase or decrease the size of the threadpool using
// an API on the Framework that allows actors, or non-actor code, to request a
// minimum or maximum limit on the number of threadpool threads.
//


#include <stdio.h>

#include <Theron/Actor.h>
#include <Theron/Framework.h>
#include <Theron/Receiver.h>


static const int PROCESSOR_ACTORS = 10;
static const int REQUESTS_PER_ACTOR = 100000;
static const int REQUEST_BATCH_SIZE = 50;


// Does some processing in response to messages, and then sends back the result.
class Processor : public Theron::Actor
{
public:

    inline Processor()
    {
        RegisterHandler(this, &Processor::Process);
    }

private:

    void Process(const bool &/*message*/, const Theron::Address from)
    {
        int total(0);
        for (int count = 0; count < 5000; ++count)
        {
            total += count;
        }

        Send(total, from);
    }
};


// Manager actor that manages the size of the framework's threadpool.
// We do this in an actor in this example just to show it can be done that way.
class Manager : public Theron::Actor
{
public:

    inline Manager() : mNumThreads(1), mCount(0)
    {
        RegisterHandler(this, &Manager::Manage);
    }

private:

    void Manage(const bool &/*message*/, const Theron::Address /*from*/)
    {
        Theron::Framework &framework(GetFramework());

        // Query the framework for counter values measuring the threadpool behavior.
        // The first counts how many times the threadpool was pulsed to wake a worker thread,
        // which happens when a message arrives at an actor that isn't already being processed.
        // The second counts how many worker threads were woken, typically as a result of being pulsed.
        // The difference between them indicates roughly how many times no sleeping thread was available.
        if (framework.GetCounterValue(Theron::Framework::COUNTER_THREADS_PULSED) >
            framework.GetCounterValue(Theron::Framework::COUNTER_THREADS_WOKEN))
        {
            ++mCount;
        }
        else
        {
            if (mNumThreads > 1) --mCount;
        }

        framework.ResetCounters();

        // To avoid oscillation, we only react after a series of shortfalls or excesses.
        if (mCount >= 15)
        {
            // Request another thread.
            framework.SetMinThreads(++mNumThreads);
            mCount = 0;
        }
        else if (mCount <= -15)
        {
            // Request one fewer thread.
            framework.SetMaxThreads(--mNumThreads);
            mCount = 0;
        }
    }

    unsigned mNumThreads;                   // Number of threads we currently want.
    int mCount;                             // Counts (too few threads) minus (too many threads).
};


int main()
{
    // Create a framework instance, with just one worker thread initially.
    Theron::Framework framework(1);
    Theron::Receiver receiver;

    // Create a manager actor to manage the threadpool, and a collection of processors.
    Theron::ActorRef manager(framework.CreateActor<Manager>());
    Theron::ActorRef processors[PROCESSOR_ACTORS];

    for (int index = 0; index < PROCESSOR_ACTORS; ++index)
    {
        processors[index] = framework.CreateActor<Processor>();
    }

    // Send requests and wait for the results in a loop intended to emulate a real program.
    // In each iteration we send more requests, collect results, and resize the threadpool.
    int requests(REQUESTS_PER_ACTOR);
    int results(PROCESSOR_ACTORS * REQUESTS_PER_ACTOR);

    while (results)
    {
        // Send a batch of requests to the processors, if we have any left to send.
        // The actors are executed by the threads in the framework's threadpool.
        // The number of actors executed at once is limited by the size of the pool.
        for (int count = 0; count < REQUEST_BATCH_SIZE && requests > 0; ++count, --requests)
        {
            for (int index = 0; index < PROCESSOR_ACTORS; ++index)
            {
                framework.Send(true, receiver.GetAddress(), processors[index].GetAddress());
            }
        }

        // Tell the manager to update the threadpool size.
        framework.Send(true, receiver.GetAddress(), manager.GetAddress());

        // Consume any new results that have arrived, without waiting.
        results -= receiver.Consume(results);
        printf("\rThreads: %d\tResults: %d\t\t", framework.GetNumThreads(), results);
    }

    printf("\nPeak threads: %d\n", framework.GetPeakThreads());
}

