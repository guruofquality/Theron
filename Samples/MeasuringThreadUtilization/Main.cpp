// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


//
// This sample demonstrates an API by which the thread utilization of a Framework
// object can be measured. It can be used as the basis for a custom control scheme
// to actively manage the number of threads active in the Framework's threadpool.
// See the SettingTheThreadCount sample for a demonstration of instructing the
// Framework to start or stop threads, and the ManagingTheThreadpool demo for an
// example of a custom threadpool management scheme implemented as an actor.
//


#include <stdio.h>

#include <Theron/Actor.h>
#include <Theron/ActorRef.h>
#include <Theron/Framework.h>
#include <Theron/Receiver.h>


static const int RESPONDER_ACTORS = 10;


// A simple actor that just returns message sent to it.
class Responder : public Theron::Actor
{
public:

    inline Responder()
    {
        RegisterHandler(this, &Responder::Respond);
    }

private:

    void Respond(const int &message, const Theron::Address from)
    {
        TailSend(message, from);
    }
};


int main()
{
    // Create a framework instance with a threadpool of 5 worker threads.
    // These are software threads and are executed on the available hardware cores.
    // They're used to execute the message handlers of actors created in this framework.
    Theron::Framework framework(5);
    Theron::Receiver receiver;

    // Create a number of Responder actors within the framework.
    Theron::ActorRef responders[RESPONDER_ACTORS];
    for (int index = 0; index < RESPONDER_ACTORS; ++index)
    {
        responders[index] = framework.CreateActor<Responder>();
    }

    // Reset the framework's counters to zero. These internal counters count thread
    // usage events and are what we'll use to measure thread utilization.
    // The counters are always enabled and incremented, and don't need to be turned on.
    framework.ResetCounters();

    // Send each responder a message. The execute in parallel using the available threads.
    for (int index = 0; index < RESPONDER_ACTORS; ++index)
    {
        framework.Send(0, receiver.GetAddress(), responders[index].GetAddress());
    }

    // Wait for the replies from the responders to ensure they've run.
    for (int index = 0; index < RESPONDER_ACTORS; ++index)
    {
        receiver.Wait();
    }

    // Query the framework for counter values measuring the threadpool behavior.
    // This counter simply measures how many messages were processed by the framework.
    // These messages are messages that arrived at actors owned by the framework,
    // including ones that arrived while those actors were already being executed,
    // and even those for which no message handler was registered.
    const int numMessages(framework.GetCounterValue(Theron::Framework::COUNTER_MESSAGES_PROCESSED));
    printf("Messages processed: %d\n", numMessages);

    // Here we query two counters that measure thread utilization more directly.
    // The first counts how many times the threadpool was pulsed to wake a worker thread,
    // which happens when a message arrives at an actor that isn't already being processed,
    // so that any message handlers registered for the message type can be executed.
    // The second counts how many worker threads were actually woken. Threads can be woken
    // for two reasons in general: in response to arrived messages, as counted by the first
    // counter, and on termination of the framework. We're interested in the first type, and
    // don't expect to see any of the second here becuase the framework is still alive.
    // We can think of the second counter as measuring how many thread invocations actually
    // did the work represented by the first counter.
    const int numPulsed(framework.GetCounterValue(Theron::Framework::COUNTER_THREADS_PULSED));
    const int numWoken(framework.GetCounterValue(Theron::Framework::COUNTER_THREADS_WOKEN));

    // The difference between the two counter values indicates roughly how many times
    // no sleeping thread was available to be woken, when a message arrived that could
    // otherwise have been processed in parallel with actors already in execution.
    // In such cases the new message is instead processed by one of the threads that is
    // already awake, when it finishes whatever it is currently doing. If the difference
    // is significant this suggests that adding more threads would allow more parallelism
    // in the application. It's important to note however that, no matter how many
    // software threads are created, they are still executed by the limited number of
    // hardware cores available (often only two). Once the available cores are exhausted
    // the parallism bought is timesliced multithreading rather than true concurrency.
    // On the other hand, this can still be valuable because it allows actors to begin
    // execution as soon as messages arrive, rather than having to wait until threads
    // become available. This can increase interactivity and apparent parallelism, at the
    // expense of more thread-switching overheads. The correct number of threads to use
    // depends not only on the number of hardware cores but also on the application:
    // ideally we'd like important messages to be handled quickly even if other actors,
    // with heavyweight message handlers that take a while to run, are already being
    // processed.
    printf("Threads pulsed: %d\nThreads woken: %d\n", numPulsed, numWoken);

    return 0;
}

