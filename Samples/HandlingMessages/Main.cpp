// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


//
// This sample shows an actor can handle messages of multiple types.
//


#include <stdio.h>

#include <Theron/Actor.h>
#include <Theron/Framework.h>
#include <Theron/Receiver.h>


// First of two message types accepted by the actor.
struct IntegerMessage
{
    inline explicit IntegerMessage(const int value) : mValue(value)
    {
    }
        
    int mValue;
};


// Second of two message types accepted by the actor.
struct FloatMessage
{
    inline explicit FloatMessage(const float value) : mValue(value)
    {
    }

    float mValue;
};


// A simple actor that handles messages of two different types.
class SimpleActor : public Theron::Actor
{
public:

    inline SimpleActor()
    {
        // Register the message handlers.
        // Here we register two handlers for messages of different types.
        // The message types handled by the handlers are implied by the types
        // they accept as parameters.
        // Handlers are automatically deregistered on destruction of the actor,
        // however we can register or deregister them explicitly at any time.
        RegisterHandler(this, &SimpleActor::IntegerHandler);
        RegisterHandler(this, &SimpleActor::FloatHandlerOne);

        // Here we register a second handler for messages of type FloatMessage.
        // Whenever a message arrives, all handlers registered for messages
        // of that type are executed (in an arbitrary order).
        RegisterHandler(this, &SimpleActor::FloatHandlerTwo);
    }

private:

    // Handler function for messages of type IntegerMessage.
    // Note that handlers must take their message parameters by reference!
    inline void IntegerHandler(const IntegerMessage &message, const Theron::Address from)
    {
        printf("IntegerHandler received message with contents '%d'\n", message.mValue);
        Send(message, from);
    }

    // First handler function for messages of type FloatMessage.
    inline void FloatHandlerOne(const FloatMessage &message, const Theron::Address from)
    {
        printf("FloatHandlerOne received message with contents '%f'\n", message.mValue);
        Send(message, from);
    }

    // Second handler function for messages of type FloatMessage.
    inline void FloatHandlerTwo(const FloatMessage &message, const Theron::Address from)
    {
        printf("FloatHandlerTwo received message with contents '%f'\n", message.mValue);
        Send(message, from);
    }
};


int main()
{
    Theron::Framework framework;
    Theron::ActorRef simpleActor(framework.CreateActor<SimpleActor>());

    Theron::Receiver receiver;

    // Send one messages of each type to the actor.
    const Theron::Address fromAddress(receiver.GetAddress());
    simpleActor.Push(IntegerMessage(10), fromAddress);
    simpleActor.Push(FloatMessage(5.0f), fromAddress);

    // Wait for the all three reply messages.
    receiver.Wait();
    receiver.Wait();
    receiver.Wait();

    printf("Received three reply messages\n");

    return 0;
}

