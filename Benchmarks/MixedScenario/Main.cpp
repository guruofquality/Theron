// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


//
// This benchmark is an implementation of a custom benchmark for which implementations
// in other systems are available on the internet. It is included for comparison
// purposes.
//
// The mixed-scenario benchmark is a hybrid benchmark which mixes real processing,
// or 'work', with synthetic stress-tests of message passing and synchronization
// overheads. It bears some similarity to ThreadRing and ParallelThreadRing, but
// is also quite different in that -- unlike those benchmarks -- it also involves a
// substantial amount of parallelizable computation, so responds well in general
// to any increase in the number of available of hardware threads.
//
// A single Supervisor actor collects and counts messages sent to it by a set
// of 20 Master actors. Each Master creates and manages both a ring of 49 connected
// ChainLink actors and a single Worker actor, effectively forming a compound
// subsystem. Each Master completes 5 iterations, where an iteration consists of
// initiating some work by its owned Worker and the passing of an integer token
// around the ring of connected ChainLink actors (with the Master itself serving
// as the 50th actor in the ring). The token has initial value of 10000, and is
// decremented by one each time it arrives back at the Master on completing a
// circuit of the ring. The receipt of a token with zero value by the Master
// signifies that 10000 cycles of the ring have been completed, whereupon the
// Master begins the next iteration, initiating another piece of work by its associated
// Worker and passing another token with initial value of 10000 into its associated
// ring of ChainLinks. Each piece of work performed by the Worker actors consists of
// factorizing a large integer, which is the product of two known primes. On
// completion of each factorization, a worker sends its factor results in a message to
// the Supervisor. The Master actors also each send a completion message to the
// Supervisor on completion of their fifth iteration. Finally, the Supervisor signifies
// termination of all processing on receiving 20 * 5 + 20 messages: namely the 5
// results sent from each Worker on completion of their factorizations, plus the
// single signal sent from each Master on completion of its iterations.
//
// The complex nature of the benchmark makes it difficult to interpret its results.
// In particular, if the computation is bound by the factorization of the primes
// by the Worker actors -- as is often the case -- then any reduction in the
// raw message passing and synchronization overheads of the underlying system
// are hidden and make no difference to the final result. Effectively the benchmark
// then measures the effectiveness with which the 100 total factorizations can be
// performed in parallel.
//


#include <stdio.h>
#include <stdlib.h>
#include <vector>

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


static const Theron::uint64_t PRIME_FACTOR_1 = 86028157ULL;
static const Theron::uint64_t PRIME_FACTOR_2 = 329545133ULL;
static const Theron::uint64_t PRIME_PRODUCT = PRIME_FACTOR_1 * PRIME_FACTOR_2;


typedef int Token;
typedef Theron::uint64_t Work;
typedef std::vector<Theron::uint64_t> Result;
struct MasterDone { };
typedef bool SupervisorResult;


// Register the message types so that registered names are used instead of dynamic_cast.
THERON_REGISTER_MESSAGE(Token);
THERON_REGISTER_MESSAGE(Work);
THERON_REGISTER_MESSAGE(MasterDone);
THERON_REGISTER_MESSAGE(Result);
THERON_REGISTER_MESSAGE(SupervisorResult);


static void Factorize(Theron::uint64_t n, std::vector<Theron::uint64_t> &results)
{
    if (n <= 3)
    {
        results.push_back(n);
        return;
    }

    Theron::uint64_t d = 2;
    while (d < n)
    {
        if ((n % d) == 0)
        {
            results.push_back(d);
            n /= d;
        }
        else
        {
            d = (d == 2) ? 3 : (d + 2);
        }
    }

    results.push_back(d);
}


class Worker : public Theron::Actor
{
public:

    typedef Theron::Address Parameters;

    inline Worker(const Parameters &params) : mSupervisor(params)
    {
        RegisterHandler(this, &Worker::WorkHandler);
    }

private:

    inline void WorkHandler(const Work &message, const Theron::Address /*from*/)
    {
        // Do some work.
        Result result;
        Factorize(Theron::uint64_t(message), result);

        // Send our result to the supervisor.
        Send(result, mSupervisor);
    }

    Theron::Address mSupervisor;
};


class ChainLink : public Theron::Actor
{
public:

    typedef Theron::Address Parameters;

    inline explicit ChainLink(const Parameters &params) : mNext(params)
    {
        RegisterHandler(this, &ChainLink::TokenHandler);
    }

private:

    inline void TokenHandler(const Token &token, const Theron::Address /*from*/)
    {
        TailSend(token, mNext);
    }

    Theron::Address mNext;
};


class Master : public Theron::Actor
{
public:

    typedef struct
    {
        Theron::Address messageCollector;
        int numCycles;
        int numLinks;
        int numIterations;
    }
    Parameters;

    inline explicit Master(const Parameters &params) :
      mSupervisor(params.messageCollector),
      mNumCycles(params.numCycles),
      mNumLinks(params.numLinks),
      mIterations(params.numIterations)
    {
        RegisterHandler(this, &Master::TokenHandler);

        // Create the worker owned by this master and give it the address to send its results.
        mWorker = GetFramework().CreateActor<Worker>(mSupervisor);

        // Start the first iteration.
        StartIteration();
    }

private:

    inline void TokenHandler(const Token &token, const Theron::Address /*from*/)
    {
        // Because we decrement the token but the chain links don't, the token count
        // how many times it has been sent around the ring.
        const Token newToken(token - 1);
        if (newToken > 0)
        {
            // We decrement the token and send it back out into the chain.
            TailSend(newToken, mNext);
        }
        else
        {
            // When the token reaches zero the current iteration has finished.
            // If there are more iterations to be done we start the next one.
            if (!StartIteration())
            {
                // Notify the supervisor that our iterations are finished.
                // The owned worker sends the collector its results independently.
                TailSend(MasterDone(), mSupervisor);
            }
        }
    }

    inline bool StartIteration()
    {
        if (mIterations--)
        {
            // Start some work for this iteration, which the worker does in parallel.
            mWorker.Push(Work(PRIME_PRODUCT), GetAddress());

            mNext = GetAddress();
            mChainLinks.clear();

            // Start at one because we only need to create numLinks - 1 links, since we're the last link.
            for (int index = 1; index < mNumLinks; ++index)
            {
                mChainLinks.push_back(GetFramework().CreateActor<ChainLink>(mNext));
                mNext = mChainLinks.back().GetAddress();
            }

            // Send an initial token to the first link in the chain.
            Send(Token(mNumCycles), mNext);

            return true;
        }

        return false;
    }

    std::vector<Theron::ActorRef> mChainLinks;
    Theron::ActorRef mWorker;
    Theron::Address mNext;
    Theron::Address mSupervisor;
    int mNumCycles;
    int mNumLinks;
    int mIterations;
};


class Supervisor : public Theron::Actor
{
public:

    typedef struct  
    {
        Theron::Address client;
        int expectedMessages;
    }
    Parameters;

    inline Supervisor(const Parameters &params) : mClient(params.client), mExpectedMessages(params.expectedMessages), mOkay(true)
    {
        RegisterHandler(this, &Supervisor::ResultHandler);
        RegisterHandler(this, &Supervisor::MasterDoneHandler);
    }

private:

    inline void ResultHandler(const Result &message, const Theron::Address /*from*/)
    {
        // The factorization should always return the two known factors.
        if (message.size() != 2 || message[0] != PRIME_FACTOR_1 || message[1] != PRIME_FACTOR_2)
        {
            mOkay = false;
        }

        if (--mExpectedMessages == 0)
        {
            TailSend(SupervisorResult(mOkay), mClient);
        }
    }

    inline void MasterDoneHandler(const MasterDone &/*message*/, const Theron::Address /*from*/)
    {
        if (--mExpectedMessages == 0)
        {
            TailSend(SupervisorResult(mOkay), mClient);
        }
    }

    Theron::Address mClient;
    int mExpectedMessages;
    bool mOkay;
};


struct ResultCatcher
{
    inline ResultCatcher() : mOkay(false)
    {
    }

    inline void Catch(const bool &value, const Theron::Address /*from*/) { mOkay = value; }

    int mOkay;
};


int main(int argc, char * argv[])
{
    int numMessagesProcessed(0), numThreadsPulsed(0), numThreadsWoken(0);
    ResultCatcher resultCatcher;

    const int numRings = (argc > 1 && atoi(argv[1]) > 0) ? atoi(argv[1]) : 20;
    const int numIterations = (argc > 2 && atoi(argv[2]) > 0) ? atoi(argv[2]) : 5;
    const int numLinks = (argc > 3 && atoi(argv[3]) > 0) ? atoi(argv[3]) : 50;
    const int numCycles = (argc > 4 && atoi(argv[4]) > 0) ? atoi(argv[4]) : 10000;
    const int numThreads = (argc > 5 && atoi(argv[5]) > 0) ? atoi(argv[5]) : 16;

    printf("Using %d rings (use first command line argument to change)\n", numRings);
    printf("Using %d iterations per ring (use second command line argument to change)\n", numIterations);
    printf("Using %d links per ring (use third command line argument to change)\n", numLinks);
    printf("Using %d cycles per iteration (use fourth command line argument to change)\n", numCycles);
    printf("Using %d worker threads (use fifth command line argument to change)\n", numThreads);

    printf("Starting %d rings...\n", numRings);

    // The reported time includes the startup and cleanup cost.
    Timer timer;
    timer.Start();

    {
        Theron::Framework framework(16);
        Theron::Receiver receiver;
        receiver.RegisterHandler(&resultCatcher, &ResultCatcher::Catch);

        // Each worker sends a result per ring iteration, plus the rings each send a done message.
        Supervisor::Parameters supervisorParams;
        supervisorParams.client = receiver.GetAddress();
        supervisorParams.expectedMessages = numRings + numRings * numIterations;

        Theron::ActorRef supervisor(framework.CreateActor<Supervisor>(supervisorParams));

        // Create a number of masters, each of which owns a chained ring and a worker.
        Master::Parameters masterParams;
        masterParams.messageCollector = supervisor.GetAddress();
        masterParams.numCycles = numCycles;
        masterParams.numIterations = numIterations;
        masterParams.numLinks = numLinks;

        std::vector<Theron::ActorRef> masters;
        for (int i = 0; i < numRings; ++i)
        {
            masters.push_back(framework.CreateActor<Master>(masterParams));
        }

        // Wait for the supervisor to tell us we're done.
        receiver.Wait();

        numMessagesProcessed = framework.GetCounterValue(Theron::Framework::COUNTER_MESSAGES_PROCESSED);
        numThreadsPulsed = framework.GetCounterValue(Theron::Framework::COUNTER_THREADS_PULSED);
        numThreadsWoken = framework.GetCounterValue(Theron::Framework::COUNTER_THREADS_WOKEN);
    }

    timer.Stop();

    printf("Result: %s\n", resultCatcher.mOkay ? "Okay" : "Failed");
    printf("Processed %d messages in %.1f seconds\n", numMessagesProcessed, timer.Seconds());
    printf("Threads pulsed: %d, woken: %d\n", numThreadsPulsed, numThreadsWoken);

#if THERON_ENABLE_DEFAULTALLOCATOR_CHECKS
    Theron::IAllocator *const allocator(Theron::AllocatorManager::Instance().GetAllocator());
    const int peakBytesAllocated(static_cast<Theron::DefaultAllocator *>(allocator)->GetPeakBytesAllocated());
    printf("Peak memory usage in bytes: %d bytes\n", peakBytesAllocated);
#endif // THERON_ENABLE_DEFAULTALLOCATOR_CHECKS

}

