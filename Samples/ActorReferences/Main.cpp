// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


//
// This sample shows how to use actor references in non-actor code.
//


#include <stdio.h>

#include <Theron/Detail/Threading/Lock.h>
#include <Theron/Detail/Threading/Mutex.h>

#include <Theron/Framework.h>
#include <Theron/Actor.h>
#include <Theron/Address.h>


struct Counter
{
    explicit Counter(const int count) : mCount(count)
    {
    }

    int mCount;
    Theron::Detail::Mutex mMutex;
};


// A simple actor that increments a referenced counter on construction and destruction.
class SimpleActor : public Theron::Actor
{
public:

    struct Parameters
    {
        inline explicit Parameters(Counter *const counter) : mCounter(counter)
        {
        }

        Counter *mCounter;
    };

    inline explicit SimpleActor(const Parameters &params) : mCounter(params.mCounter)
    {
        // Increment the referenced counter on construction.
        Theron::Detail::Lock lock(mCounter->mMutex);
        ++mCounter->mCount;
    }

    inline virtual ~SimpleActor()
    {
        // Increment the referenced counter again on destruction.
        Theron::Detail::Lock lock(mCounter->mMutex);
        ++mCounter->mCount;
    }

private:

    Counter *const mCounter;
};


// An example of a function that accepts an actor reference by value.
// Actor references are lightweight and copyable so can safely be passed
// and returned by value. Another thing to note is that the ActorRef type
// is loosely typed and can reference an actor of any type.
void PushMessage(Theron::ActorRef actor)
{
    // Push an arbitrary message into the actor.
    actor.Push(int(5), Theron::Address());
}


int main()
{
    Theron::Framework framework;

    // A shared threadsafe counter, initialized to zero.
    Counter counter(0);

    // We use a local scope here to force destruction of the actor.
    {
        // Create a single instance of an actor, passing the address of the count.
        // We get back a reference to the actor rather than the actor itself.
        // The reference is lightweight and can be copied and assigned.
        const SimpleActor::Parameters params(&counter);
        Theron::ActorRef actorRef(framework.CreateActor<SimpleActor>(params));

        // Copy the reference, producing a second reference to the same actor.
        // Note that the actor itself is not copied, just the reference.
        Theron::ActorRef refTwo(actorRef);

        // Call a function, passing the actor reference by value.
        // Because copying a reference doesn't copy the referenced actor,
        // this doesn't amount to constructing and destructing new actors.
        PushMessage(refTwo);

        // To prove that no further actors have been constructed or destructed,
        // we check the shared counter has only been incremented once, on
        // construction of the original actor.
        {
            Theron::Detail::Lock lock(counter.mMutex);
            if (counter.mCount != 1)
            {
                printf("Counter value incorrect\n");
            }
        }    
        
        // The last actor reference is destructed here, causing the actor
        // itself to become unreferenced and scheduled for destruction.
    }

    // Check that the counter is eventually incremented again on destruction
    // of the actor. Actors that are scheduled for destruction are actually
    // destructed asynchronously by the worker threads, using a process called
    // Garbage Collection, so we can't be sure exactly when it will happen.
    // For that reason in this simple example we use a busy-waiting loop
    // to repeatedly check the counter until it happens. We wouldn't normally
    // do this.

    int counterValue(1);
    while (counterValue != 2)
    {
        Theron::Detail::Lock lock(counter.mMutex);
        counterValue = counter.mCount;
    }

    printf("Counter value correct\n");
    return 0;
}

