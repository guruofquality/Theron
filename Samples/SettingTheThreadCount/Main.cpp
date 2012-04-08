// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


//
// This sample demonstrates an API by which the size of the threadpool owned by
// a Framework object can be controlled. It can be used as the basis for a custom
// control scheme to manage the number of threads active in the Framework's
// threadpool in software at runtime. See the MeasuringThreadUtilizatio sample for
// a demonstration of a related API by which your application can measure the degree
// to which the existing threads in the threadpool are being utilitized, and hence
// decide whether to add or remove threads. See also the ManagingTheThreadpool demo
// for an example of a custom threadpool management scheme implemented as an actor.
//


#include <stdio.h>

#include <Theron/Actor.h>
#include <Theron/ActorRef.h>
#include <Theron/Framework.h>


int main()
{
    // Create a framework instance with a threadpool of 2 worker threads by default
    // The threads are used to execute the message handlers of actors created in this framework.
    Theron::Framework framework;

    // Create a dummy actor as a target for messages. It doesn't handle the messages or respond.
    Theron::ActorRef dummyActor(framework.CreateActor<Theron::Actor>());

    // Here we call various methods on the Framework that together constitute an
    // API for querying and controlling the number of threads in its threadpool.

    // This first method queries the actual number of threads enabled at this moment.
    // Note that this counts all enabled threads and includes any that are sleeping
    // due to having no work to do.
    printf("Thread count initially: %d\n", framework.GetNumThreads());

    // This method allows an application to set the thread count to a required minimum.
    // This is guaranteed to eventually result in the number of threads being greater
    // than or equal to this number, unless a lower maximum is specified subsequently.
    framework.SetMinThreads(10);
    printf("Thread count immediately after SetMinThreads(10): %d\n", framework.GetNumThreads());

    // Note that it can take a while for the actual number of threads to reach the
    // minimum. Threads are spawned or re-enabled by a manager thread dedicated to that
    // task, which runs asynchronously from other threads as a background task.
    // It spends most of its time asleep, only being woken by calls like this one.
    // For the sake of the example, we call the method many times to make this happen.
    for (int count = 0; count < 10; ++count)
    {
        framework.GetNumThreads();
    }

    printf("Thread count subsequently: %d\n", framework.GetNumThreads());

    // This similar call allows an application to place an upper limit on the thread count.
    // This is guaranteed to eventually result in the number of threads being less than
    // or equal to this number, as long as messages continue to be sent and unless a higher
    // maximum is specified subsequently. The idea behind separate minimum and maximum limits,
    // rather than a single method to directly set the actual number of threads, is to allow
    // negotiation between multiple agents, each with a different interest in the thread count.
    // One may require a certain minimum number of threads for its processing, but not care if
    // the actual number of threads is higher, while another may wish to impose a maximum limit
    // on the number of threads in existance, but be satisfied if there are less. If two calls
    // specify different minimums, the greater takes effect. Likewise if two maximums are
    // specified, the lower is effective. If conflicting minimum and maximums are specified
    // by subsequent calls, then the later call wins.
    framework.SetMaxThreads(5);
    printf("Thread count immediately after SetMaxThreads(5): %d\n", framework.GetNumThreads());

    // Note that again the thread count can take a while to reach the specified level. Threads
    // only terminate on being woken, and are not actively terminated by the manager thread.
    // This means that until some threads are woken by the arrival of new messages, the actual
    // thread count will remain unchanged. Here we send messages to a dummy actor, causing threads
    // to be woken and terminate.
    for (int count = 0; count < 10; ++count)
    {
        framework.Send(0, Theron::Address(), dummyActor.GetAddress());
    }

    printf("Thread count subsequently: %d\n", framework.GetNumThreads());

    // These methods query the current minimum and maximum thread counts.
    // Note that these are the currently active minimum and maximum limits, and are
    // not guaranteed to match any limits specified earlier. In particular, in the
    // current implementation the effective minimum and maximum limits are always
    // the same, and effectively specify a target thread count.
    printf("Current minimum thread count limit: %d\n", framework.GetMinThreads());
    printf("Current maximum thread count limit: %d\n", framework.GetMaxThreads());

    // This call queries the highest number of simultaneously enabled threads seen since the
    // start of the framework. Note that this measures the highest actual number of threads,
    // as measured by GetNumThreads, rather than the highest values of the maximum or minimum
    // thread count limits. Finally note that this call, like all the others shown here,
    // is specific to this framework instance. If multiple frameworks are created
    // then each has its own threadpool with an independently managed thread count.
    printf("Peak thread count: %d\n", framework.GetPeakThreads());

    return 0;
}

