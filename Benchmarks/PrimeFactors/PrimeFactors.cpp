// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#include <stdio.h>
#include <stdlib.h>

#include <vector>
#include <queue>

#include <Theron/Theron.h>

#include "../Common/Timer.h"


static const Theron::uint64_t PRIME_FACTOR_1 = 20483ULL;
static const Theron::uint64_t PRIME_FACTOR_2 = 29303ULL;
static const Theron::uint64_t PRIME_PRODUCT = PRIME_FACTOR_1 * PRIME_FACTOR_2;


struct QueryMessage
{
    inline QueryMessage(const Theron::Address &client, const Theron::uint64_t integer) :
      m_client(client),
      m_integer(integer),
      m_factor(0)
    {
    }

    inline void Process()
    {
        const Theron::uint64_t n(m_integer);

        if (n <= 3)
        {
            m_factor = n;
            return;
        }

        Theron::uint64_t d = 2;
        while (d < n)
        {
            if ((n % d) == 0)
            {
                m_factor = d;
                return;
            }

            d = (d == 2) ? 3 : (d + 2);
        }
    }

    inline const Theron::Address &Client() const
    {
        return m_client;
    }

    inline bool Processed() const
    {
        return (m_factor != 0);
    }

    Theron::Address m_client;
    Theron::uint64_t m_integer;
    Theron::uint64_t m_factor;
};


// Register the message types so that registered names are used instead of dynamic_cast.
THERON_DECLARE_REGISTERED_MESSAGE(QueryMessage);
THERON_DEFINE_REGISTERED_MESSAGE(QueryMessage);


// A stateless worker actor that processes work messages.
// Each worker can only process one work item at a time.
class Worker : public Theron::Actor
{
public:

    inline Worker(Theron::Framework &framework) : Theron::Actor(framework)
    {
        RegisterHandler(this, &Worker::Handler);
    }

private:

    inline void Handler(const QueryMessage &query, const Theron::Address from)
    {
        // The query parameter is const so we need to copy it to change it.
        QueryMessage result(query);
        result.Process();
        Send(result, from);
    }
};


// A dispatcher actor that processes work items.
// Internally the dispatcher creates and controls a pool of workers.
// It coordinates the workers to process the work items in parallel.
class Dispatcher : public Theron::Actor
{
public:

    inline Dispatcher(Theron::Framework &framework, const int workerCount) : Theron::Actor(framework)
    {
        // Create the workers and add them to the free list.
        for (int i = 0; i < workerCount; ++i)
        {
            mWorkers.push_back(new Worker(framework));
            mFreeQueue.push(mWorkers.back()->GetAddress());
        }

        RegisterHandler(this, &Dispatcher::Handler);
    }

    inline ~Dispatcher()
    {
        // Destroy the workers.
        const int workerCount(static_cast<int>(mWorkers.size()));
        for (int i = 0; i < workerCount; ++i)
        {
            delete mWorkers[i];
        }
    }

private:

    inline void Handler(const QueryMessage &query, const Theron::Address from)
    {
        // Has this work item been processed?
        if (query.Processed())
        {
            // Send the result back to the caller that requested it.
            Send(query, query.Client());

            // Add the worker that sent the result to the free list.
            mFreeQueue.push(from);
        }
        else
        {
            // Add the unprocessed query to the work list.
            mWorkQueue.push(query);
        }

        // Service the work queue.
        if (!mWorkQueue.empty() && !mFreeQueue.empty())
        {
            Send(mWorkQueue.front(), mFreeQueue.front());

            mFreeQueue.pop();
            mWorkQueue.pop();
        }
    }

    std::vector<Worker *> mWorkers;             // Pointers to the owned workers.
    std::queue<Theron::Address> mFreeQueue;     // Queue of available workers.
    std::queue<QueryMessage> mWorkQueue;        // Queue of unprocessed work messages.
};


int main(int argc, char *argv[])
{
    int numMessagesProcessed(0), numYields(0), numLocalPushes(0), numSharedPushes(0);

    const int numQueries = (argc > 1 && atoi(argv[1]) > 0) ? atoi(argv[1]) : 1000000;
    const int numThreads = (argc > 2 && atoi(argv[2]) > 0) ? atoi(argv[2]) : 16;
    const int numWorkers = (argc > 3 && atoi(argv[3]) > 0) ? atoi(argv[3]) : 16;

    printf("Using numQueries = %d (use first command line argument to change)\n", numQueries);
    printf("Using numThreads = %d (use second command line argument to change)\n", numThreads);
    printf("Using numWorkers = %d (use third command line argument to change)\n", numWorkers);

    // The reported time includes the startup and cleanup cost.
    Timer timer;
    timer.Start();

    {
        Theron::Framework framework(numThreads);
        Dispatcher dispatcher(framework, numWorkers);
        Theron::Receiver receiver;

        // Send a bunch of work items to the dispatcher for processing.
        const QueryMessage query(receiver.GetAddress(), PRIME_PRODUCT);

        int count(0);
        while (count < numQueries)
        {
            framework.Send(query, receiver.GetAddress(), dispatcher.GetAddress());
            ++count;
        }

        // Wait for all the results. We don't bother to check them.
        while (count > 0)
        {
            count -= static_cast<int>(receiver.Wait(count));
        }

        numMessagesProcessed = framework.GetCounterValue(Theron::COUNTER_MESSAGES_PROCESSED);
        numYields = framework.GetCounterValue(Theron::COUNTER_YIELDS);
        numLocalPushes = framework.GetCounterValue(Theron::COUNTER_LOCAL_PUSHES);
        numSharedPushes = framework.GetCounterValue(Theron::COUNTER_SHARED_PUSHES);
    }

    timer.Stop();

    printf("Processed %d messages in %.1f seconds\n", numMessagesProcessed, timer.Seconds());
    printf("Counted %d thread yields, %d local pushes and %d shared pushes\n", numYields, numLocalPushes, numSharedPushes);

#if THERON_ENABLE_DEFAULTALLOCATOR_CHECKS
    Theron::IAllocator *const allocator(Theron::AllocatorManager::Instance().GetAllocator());
    const int allocationCount(static_cast<Theron::DefaultAllocator *>(allocator)->GetAllocationCount());
    const int peakBytesAllocated(static_cast<Theron::DefaultAllocator *>(allocator)->GetPeakBytesAllocated());
    printf("Total number of allocations: %d calls\n", allocationCount);
    printf("Peak memory usage in bytes: %d bytes\n", peakBytesAllocated);
#endif // THERON_ENABLE_DEFAULTALLOCATOR_CHECKS

}

