// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


//
// This benchmark measures the speed of sending a large number of messages from
// one or more actors (the producers) to another (the consumer). One of the things
// measured by this is the effectiveness of memory block caching, used to ensure that
// memory allocated for messages, and message queue nodes, is reused rather than
// continually freed and reallocated via the main allocator (which is typically slow).
// Note that for large numbers of messages, this benchmark may be memory-limited.
//


#include <stdio.h>
#include <vector>

// Enable checking for unregistered message types.
#define THERON_ENABLE_MESSAGE_REGISTRATION_CHECKS 1

#include <Theron/Actor.h>
#include <Theron/AllocatorManager.h>
#include <Theron/DefaultAllocator.h>
#include <Theron/Framework.h>
#include <Theron/Receiver.h>
#include <Theron/Register.h>

#include "../Common/Timer.h"


class Producer : public Theron::Actor
{
public:

    struct StartMessage
    {
        inline StartMessage(const Theron::Address &consumer, const int count) : mConsumer(consumer), mCount(count)
        {
        }

        Theron::Address mConsumer;
        int mCount;
    };

    inline Producer()
    {
        RegisterHandler(this, &Producer::Start);
    }

private:

    inline void Start(const StartMessage &message, const Theron::Address /*from*/)
    {
        int count(message.mCount);
        while (count--)
        {
            Send(count, message.mConsumer);
        }
    }
};


class Consumer : public Theron::Actor
{
public:

    struct StartMessage
    {
        inline StartMessage(const Theron::Address &caller, const int count) : mCaller(caller), mCount(count)
        {
        }

        Theron::Address mCaller;
        int mCount;
    };

    inline Consumer() : mCaller(Theron::Address::Null()), mCount(0)
    {
        RegisterHandler(this, &Consumer::Start);
    }

private:

    inline void Start(const StartMessage &message, const Theron::Address /*from*/)
    {
        mCaller = message.mCaller;
        mCount = message.mCount;

        DeregisterHandler(this, &Consumer::Start);
        RegisterHandler(this, &Consumer::Consume);
    }

    inline void Consume(const int &/*message*/, const Theron::Address /*from*/)
    {
        if (--mCount == 0)
        {
            Send(0, mCaller);
        }
    }

    Theron::Address mCaller;
    int mCount;
};


// Register the message types so that registered names are used instead of dynamic_cast.
THERON_REGISTER_MESSAGE(int);
THERON_REGISTER_MESSAGE(Producer::StartMessage);
THERON_REGISTER_MESSAGE(Consumer::StartMessage);


int main(int argc, char *argv[])
{
    int numMessagesProcessed(0), numThreadsPulsed(0), numThreadsWoken(0);
    const int numMessages = (argc > 1 && atoi(argv[1]) > 0) ? atoi(argv[1]) : 50000000;
    const int numProducers = (argc > 2 && atoi(argv[2]) > 0) ? atoi(argv[2]) : 2;
    const int numThreads = (argc > 3 && atoi(argv[3]) > 0) ? atoi(argv[3]) : 16;
    const int messagesPerProducer((numMessages + numProducers - 1) / numProducers);

    printf("Using numMessages = %d (use first command line argument to change)\n", numMessages);
    printf("Using numProducers = %d (use second command line argument to change)\n", numProducers);
    printf("Using numThreads = %d (use third command line argument to change)\n", numThreads);
    printf("Processing...\n");

    // The reported time includes the startup and cleanup cost.
    Timer timer;
    timer.Start();

    {
        Theron::Framework framework(numThreads);
        std::vector<Theron::ActorRef> producers(numProducers);
        Theron::ActorRef consumer(framework.CreateActor<Consumer>());
        Theron::Receiver receiver;

        // Create the producers.
        for (int i = 0; i < numProducers; ++i)
        {
            producers[i] = framework.CreateActor<Producer>();
        }

        // Start the consumer, telling it the number of messages to expect and the
        // address to notify when the count is reached.
        const Consumer::StartMessage consumerStart(receiver.GetAddress(), numMessages);
        framework.Send(consumerStart, receiver.GetAddress(), consumer.GetAddress());

        // Start the producers, telling them how many messages to send and the address of the consumer.
        // We divide the total number of messages between the producers so they're equally loaded.
        int messagesLeft(numMessages);
        Producer::StartMessage producerStart(consumer.GetAddress(), messagesPerProducer);

        for (int i = 0; i < numProducers; ++i)
        {
            if (messagesLeft < messagesPerProducer)
            {
                producerStart.mCount = messagesLeft;
            }

            framework.Send(producerStart, receiver.GetAddress(), producers[i].GetAddress());
            messagesLeft -= producerStart.mCount;
        }

        // Wait until the consumer notifies us that it's consumed all the messages.
        receiver.Wait();

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

