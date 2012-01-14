// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


//
// This sample shows how to use multiple actor frameworks in a single app.
//


#include <stdio.h>

#include <Theron/Actor.h>
#include <Theron/ActorRef.h>
#include <Theron/Address.h>
#include <Theron/Framework.h>
#include <Theron/Receiver.h>


class SimpleActor : public Theron::Actor
{
public:

    SimpleActor()
    {
        SetDefaultHandler(this, &SimpleActor::Handler);
    }

private:

    void Handler(const Theron::Address from)
    {
        printf("Actor at '%d' received a message from '%d'\n",
            GetAddress().AsInteger(),
            from.AsInteger());
        
        // Send a message back to the sender.
        TailSend(int(0), from);
    }
};


int main()
{
    // Construct two frameworks. There's no basic limit to how many frameworks we
    // can construct in the same application. Each framework owns a pool of worker
    // threads. When we construct each framework we get to say how many worker threads
    // it has. In this case our first framework has only a single worker thread,
    // and our second framework has three worker threads.
    printf("Constructing two frameworks\n");
    Theron::Framework frameworkOne(1);
    Theron::Framework frameworkTwo(3);

    Theron::Receiver receiver;
    printf("Created a receiver with address '%d'\n", receiver.GetAddress().AsInteger());

    // We can create actors in any of the frameworks we construct. The effect of
    // creating an actor in a specific framework is to limit the execution of the
    // actor to the worker threads owned by that framework. Kind of like thread
    // affinities. In this case we're creating the actor in the first framework
    // so we know it will only ever be executed by the single worker thread owned
    // by that framework.
    Theron::ActorRef actorOne = frameworkOne.CreateActor<SimpleActor>();

    // This second actor is created in the second framework so will be executed
    // by the three worker threads in that framework.
    Theron::ActorRef actorTwo = frameworkTwo.CreateActor<SimpleActor>();

    // We can send messages to actors as normal, no matter which framework they are in.
    // Actors in different frameworks can also send messages to each other using their
    // unique addresses as normal. Addresses are globally unique across all frameworks.
    frameworkOne.Send(int(0), receiver.GetAddress(), actorOne.GetAddress());
    frameworkTwo.Send(int(0), receiver.GetAddress(), actorOne.GetAddress());
    frameworkOne.Send(int(0), receiver.GetAddress(), actorTwo.GetAddress());
    frameworkTwo.Send(int(0), receiver.GetAddress(), actorTwo.GetAddress());

    // Wait for all replies before terminating, so the frameworks don't get destructed.
    receiver.Wait();
    receiver.Wait();
    receiver.Wait();
    receiver.Wait();

    return 0;
}

