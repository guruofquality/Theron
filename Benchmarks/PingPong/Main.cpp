// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


//
// This benchmarks measures the latency of responding to messages in Theron.
// Latency refers to the delay, or elapsed time, between sending a message
// to an actor and receiving a response. Even if Theron is capabale of high throughput
// (handling millions of messages per second across a number of actors), the latency
// of handling individual messages is an independent concern and equally important for
// specialized applications where fast responses are important.
//
// The ping-pong benchmark is a standard microbenchmark commonly used to measure the
// message processing speed of concurrent systems such as computer networks.
// * Create two actors, called Ping and Pong.
// * Ping is set up to send any non-zero integer messages it receives to Pong, decremented by one.
// * Pong is set up to send any non-zero integer messages it receives to Ping, decremented by one.
// * On receipt of a zero integer message, Ping and Pong send a signal message to the client code indicating completion.
// * Processing is initiated by sending a non-zero integer message to Ping.
//
// The work done by the benchmark consists of sending n messages between Ping and Pong, where
// n is the initial value of the integer message initially sent to Ping. The latency of the
// message sending is calculated as the total execution time divided by the number of messages n.
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


class PingPong : public Theron::Actor
{
public:

    struct StartMessage
    {
        inline StartMessage(const Theron::Address &caller, const Theron::Address &partner) :
          mCaller(caller),
          mPartner(partner)
        {
        }

        Theron::Address mCaller;
        Theron::Address mPartner;
    };

    inline PingPong()
    {
        RegisterHandler(this, &PingPong::Start);
    }

private:

    inline void Start(const StartMessage &message, const Theron::Address /*from*/)
    {
        mCaller = message.mCaller;
        mPartner = message.mPartner;

        DeregisterHandler(this, &PingPong::Start);
        RegisterHandler(this, &PingPong::Receive);
    }

    inline void Receive(const int &message, const Theron::Address /*from*/)
    {
        if (message > 0)
        {
            TailSend(message - 1, mPartner);
            return;
        }

        TailSend(message, mCaller);
    }

    Theron::Address mCaller;
    Theron::Address mPartner;
};


// Register the message types so that registered names are used instead of dynamic_cast.
THERON_REGISTER_MESSAGE(int);
THERON_REGISTER_MESSAGE(PingPong::StartMessage);


int main(int argc, char *argv[])
{
    int numMessagesProcessed(0), numThreadsPulsed(0), numThreadsWoken(0);
    const int numMessages = (argc > 1 && atoi(argv[1]) > 0) ? atoi(argv[1]) : 50000000;
    const int numThreads = (argc > 2 && atoi(argv[2]) > 0) ? atoi(argv[2]) : 16;

    printf("Using numMessages = %d (use first command line argument to change)\n", numMessages);
    printf("Using numThreads = %d (use second command line argument to change)\n", numThreads);
    printf("Starting %d message sends between ping and pong...\n", numMessages);

    Theron::Framework framework(numThreads);
    Theron::Receiver receiver;

    Theron::ActorRef ping(framework.CreateActor<PingPong>());
    Theron::ActorRef pong(framework.CreateActor<PingPong>());

    // Start Ping and Pong, sending each the address of the other and the address of the receiver.
    const PingPong::StartMessage pingStart(receiver.GetAddress(), pong.GetAddress());
    framework.Send(pingStart, receiver.GetAddress(), ping.GetAddress());
    const PingPong::StartMessage pongStart(receiver.GetAddress(), ping.GetAddress());
    framework.Send(pongStart, receiver.GetAddress(), pong.GetAddress());

    Timer timer;
    timer.Start();

    // Send the initial integer count to Ping.
    framework.Send(numMessages, receiver.GetAddress(), ping.GetAddress());

    // Wait to hear back from either Ping or Pong when the count reaches zero.
    receiver.Wait();
    timer.Stop();

    numMessagesProcessed = framework.GetCounterValue(Theron::Framework::COUNTER_MESSAGES_PROCESSED);
    numThreadsPulsed = framework.GetCounterValue(Theron::Framework::COUNTER_THREADS_PULSED);
    numThreadsWoken = framework.GetCounterValue(Theron::Framework::COUNTER_THREADS_WOKEN);

    // The number of full cycles is half the number of messages.
    printf("Completed %d message response cycles\n", numMessages / 2);
    printf("Sent %d messages in %.1f seconds\n", numMessagesProcessed, timer.Seconds());
    printf("Average response time is %.10f seconds\n", timer.Seconds() / (numMessages / 2));
    printf("Threads pulsed: %d, woken: %d\n", numThreadsPulsed, numThreadsWoken);

#if THERON_ENABLE_DEFAULTALLOCATOR_CHECKS
    Theron::IAllocator *const allocator(Theron::AllocatorManager::Instance().GetAllocator());
    const int peakBytesAllocated(static_cast<Theron::DefaultAllocator *>(allocator)->GetPeakBytesAllocated());
    printf("Peak memory usage in bytes: %d bytes\n", peakBytesAllocated);
#endif // THERON_ENABLE_DEFAULTALLOCATOR_CHECKS

}

