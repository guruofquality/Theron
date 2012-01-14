// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


//
// This sample shows how to register message handlers in an actor class.
//


#include <stdio.h>

#include <Theron/Framework.h>
#include <Theron/Receiver.h>
#include <Theron/Actor.h>


struct Message
{
    inline Message(const int value) : mValue(value)
    {
    }
    
    int mValue;
};


// An example actor with two different handlers for the same message type.
class ExampleActor : public Theron::Actor
{
public:

    inline ExampleActor()
    {
        // Only handler one is registered initially.
        RegisterHandler(this, &ExampleActor::HandlerOne);
    }

private:

    inline void HandlerOne(const Message &message, const Theron::Address from)
    {
        printf("Handler ONE received message with value '%d'\n", message.mValue);
        
        // Switch to handler two
        DeregisterHandler(this, &ExampleActor::HandlerOne);
        RegisterHandler(this, &ExampleActor::HandlerTwo);
        
        Send(message, from);
    }

    inline void HandlerTwo(const Message &message, const Theron::Address from)
    {
        printf("Handler TWO received message with value '%d'\n", message.mValue);

        // Switch to handler one
        DeregisterHandler(this, &ExampleActor::HandlerTwo);
        RegisterHandler(this, &ExampleActor::HandlerOne);

        Send(message, from);
    }
};


int main()
{
    Theron::Framework framework;
    Theron::ActorRef exampleActor(framework.CreateActor<ExampleActor>());

    Theron::Receiver receiver;

    // Send a series of messages to the actor.
    // Each time it receives a message it switches handlers, so that
    // it reacts differently to odd and even messages.
    for (int count = 0; count < 10; ++count)
    {
        exampleActor.Push(Message(count), receiver.GetAddress());
    }

    // Wait for the same number of response messages.
    for (int count = 0; count < 10; ++count)
    {
        receiver.Wait();
    }

    return 0;
}

