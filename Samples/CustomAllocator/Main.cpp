// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


//
// This sample shows how to replace the default allocator in Theron.
//


#include <stdio.h>

#include <Theron/Actor.h>
#include <Theron/AllocatorManager.h>
#include <Theron/Framework.h>
#include <Theron/IAllocator.h>
#include <Theron/Receiver.h>

#include <Common/LinearAllocator.h>


// A trivial message type.
class Message
{
};


// A simple actor that just sends back any messages it receives.
class ResponderActor : public Theron::Actor
{
public:

    inline ResponderActor()
    {
        RegisterHandler(this, &ResponderActor::Handler);
    }

private:

    // Handler for messages of type Message.
    inline void Handler(const Message &message, const Theron::Address from)
    {
        // Just send the message back.
        Send(message, from);
    }
};


int main()
{
    // Construct a LinearAllocator around a memory buffer
    const unsigned int BUFFER_SIZE(16384); 
    unsigned char buffer[BUFFER_SIZE];
    Example::LinearAllocator linearAllocator(buffer, BUFFER_SIZE);

    printf("Created linear allocator with %d bytes free space\n", linearAllocator.FreeSpace());

    // Set a custom allocator for use by Theron.
    Theron::AllocatorManager::Instance().SetAllocator(&linearAllocator);

    // Construct a framework. Note the framework is allocated on the stack here.
    Theron::Framework framework;
    
    // Create an actor instance. The actor is allocated by the LinearAllocator.
    Theron::ActorRef responder(framework.CreateActor<ResponderActor>());

    // Construct a Receiver on the stack.
    Theron::Receiver receiver;

    // Send the actor a message; it sends it back.
    // The message sending and queueing uses the LinearAllocator.
    Theron::Address fromAddress(receiver.GetAddress());
    responder.Push(Message(), fromAddress);

    printf("Sent message to actor\n");

    // Wait for the reply message.
    receiver.Wait();    

    printf("Received reply from actor\n");
    printf("Linear allocator has %d bytes free space left\n", linearAllocator.FreeSpace());

    return 0;
}

