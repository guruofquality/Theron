// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


//
// This sample shows how to safely shut down a framework.
//


#include <stdio.h>

#include <Theron/Actor.h>
#include <Theron/Framework.h>
#include <Theron/Receiver.h>


// A trivial message type.
class Message
{
};


// A simple actor that sends back any messages it receives.
class SimpleActor : public Theron::Actor
{
public:

    inline SimpleActor()
    {
        RegisterHandler(this, &SimpleActor::Handler);
    }

private:

    // Handler for messages.
    inline void Handler(const Message &message, const Theron::Address from)
    {
        printf("Actor received Message from address '%d'\n", from.AsInteger());
        
        // Do some lengthy processing to simulate execution of a real actor program.
        // The actual computation here is meaningless, it's just something the compiler
        // won't optimize away.
        int count(0);
        for (unsigned int i = 0; i < 10000000; ++i)
        {
            if (i == from.AsInteger())
            {
                ++count;
            }
        }

        // This condition should always be true.
        if (count)
        {
            Send(message, from);
            printf("Actor sent Message back to address '%d'\n", from.AsInteger());
        }
    }
};


int main()
{
    printf("Starting the framework\n");

    Theron::Framework framework;
    Theron::ActorRef actor(framework.CreateActor<SimpleActor>());

    // The framework contains inside it a collection of worker threads, which
    // it uses to execute the message handlers of actors that receive messages.
    // When the framework is destructed, it stops these threads, and no further
    // message handlers are executed. Any messages still sitting in the local
    // message queues of actors will not be processed. This scenario is thread-safe
    // and will not (hopefully!) cause the program to crash. Still, it is almost
    // always not what we want. Instead, we'd like the framework to only be shut
    // down once all actor communication has completed and the actors are silent.

    // What we need is a way to signal to the main program thread (ie. the one
    // executing the main() function) that actor processing is complete and the
    // framework can be safely terminated. Without this synchronization, the main
    //  thread can easily reach the end of main() and start destructing things
    // while the actor-based computation is still ongoing.
    
    // The Receiver class is the easiest way to achieve this synchronization.
    // It is a specialized class that has an address and can accept messages from
    // actors. You can think of it as a mock actor that can receive messages
    // but can't send them, and has no internal processing of its own.
    Theron::Receiver receiver;

    // Typically one of the actors in the system is responsible for sending us a
    // signal message when processing is complete. In this example we only have one
    // actor. We supply that actor with the address to which the signal message
    // should be sent. We use the address of the receiver, causing the signal message
    // to be sent to it. The address is easily supplied as the 'from' address given
    // when pushing or sending the message. Alternatively we could supply it in a
    // specialized message, eg. StartMessage, that contains the address.
    actor.Push(Message(), receiver.GetAddress());

    // By calling the Receiver::Wait() method, we cause this thread to block until
    // the signal message is caught by the receiver. Of course it's possible that
    // the signal message actually arrived at the receiver *before* we called Wait().
    // That possibility is accounted for by a subtlety of the Wait() method: it will
    // return without waiting if a message has already been received. It maintains
    // an internal count of the number of messages that have arrived but not been
    // waited for, and will return as soon as this count is greater than zero.
    // At any rate, the important thing from our point of view is that when the
    // call to Wait() returns, we can be sure that the signal message has been
    // received and therefore that the actor processing is complete.
    receiver.Wait();

    // At this point the objects allocated on the stack will be destructed,
    // in the reverse of their construction order. In this example the receiver
    // will be destructed first, then the actor reference, then lastly the
    // framework. When the framework is shut down its worker threads will be stopped,
    // however that's okay because we are sure that no actors have queued messages,
    // so they are no longer needed.
    
    printf("Terminating the framework\n");
    
    return 0;
}

