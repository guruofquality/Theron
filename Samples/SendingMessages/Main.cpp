// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


//
// This sample shows how to send messages to and from actors.
//


#include <stdio.h>
#include <string.h>

#include <Theron/Actor.h>
#include <Theron/Address.h>
#include <Theron/Framework.h>
#include <Theron/Receiver.h>


// A custom message type containing a string.
// Note that the message contains an actual string buffer rather
// than a pointer to a string, because passing pointers in messages
// is generally unsafe: it allows actors to share pointers to the
// same memory.
struct StringMessage
{
    char mString[64];
};


// A simple actor that just receives messages and sends them back.
class SimpleActor : public Theron::Actor
{
public:

    inline SimpleActor()
    {
        // Register a message handler function.
        RegisterHandler(this, &SimpleActor::Handler);
    }

private:

    // Message handler function.
    inline void Handler(const StringMessage &message, const Theron::Address from)
    {
        printf("Received message with contents '%s'\n", message.mString);

        // We send the same message back to the sender in reply.
        // We can send messages of any type and to any address.
        // Messages are sent by value (copied) so there is no need to worry
        // about allocation and freeing of messages.
        if (!Send(message, from))
        {
            printf("Failed to send message to address %d\n", from.AsInteger());
        }

        // Send the message again, this time using TailSend.
        // The TailSend method is functionally equivalent to Send, but
        // is typically faster when executed as the last ("tail") operation of
        // a message handler. It differs from Send in the respect that it
        // doesn't wake a different worker thread to process the arrival of
        // the message at the recipient. Instead, the worker thread executing
        // this message handler will typically process it itself, when it
        // finishes executing this one. This avoids the overheads of waking
        // a sleeping thread (and putting to sleep this one), which is wasted
        // cost in cases where the current message handler is near completion
        // anyway.
        if (!TailSend(message, from))
        {
            printf("Failed to send message to address %d\n", from.AsInteger());
        }
    }
};


int main()
{
    Theron::Framework framework;
    Theron::ActorRef actor(framework.CreateActor<SimpleActor>());

    // Construct a message to send.
    StringMessage message;
    strcpy(message.mString, "Hello Theron!");

    // Create a Receiver, which lets us receive messages sent by actors.
    Theron::Receiver receiver;

    // Send the message to the actor using its unique address. We need to supply
    // a 'from' address when sending a message, and here we use the address of the
    // receiver, causing return messages to be sent to it.
    if (!framework.Send(message, receiver.GetAddress(), actor.GetAddress()))
    {
        printf("Failed to send message!\n");
    }

    // Here we show a more specialized way of sending an actor a message.
    // This method pushes a message directly into the actor, and can only be
    // used when we have an ActorRef referencing the actor. Otherwise we need
    // to use Send() to mail the actor using its address, as shown above.
    // Again we need a 'from' address, and we use the address of the receiver.
    if (!actor.Push(message, receiver.GetAddress()))
    {
        printf("Failed to push message!\n");
    }

    // Wait for all four reply messages to be received before terminating.
    receiver.Wait();
    receiver.Wait();
    receiver.Wait();
    receiver.Wait();

    printf("Received four reply messages\n");
    return 0;
}

