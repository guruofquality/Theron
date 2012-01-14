// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


//
// This sample shows how to use a fallback handler to catch unhandled messages.
//


#include <stdio.h>

#include <Theron/Actor.h>
#include <Theron/Address.h>
#include <Theron/Framework.h>
#include <Theron/Receiver.h>


// Trivial actor that ignores all messages, so that any sent to it
// are passed on the fallback handler registered with its owning framework.
class Actor : public Theron::Actor
{
};


// A simple non-trivial test message with some readable data.
struct Message
{
    Message(const int a, const float b) : mA(a), mB(b)
    {
    }

    int mA;
    float mB;
};


// Simple fallback handler object that logs unhandled messages to stdout.
// Note that this handler is not threadsafe in the sense that if called
// multiple times concurrently (which can happen) then the printf output
// of the different calls will be interleaved. We live with that here, but
// if the class contained member data (for example a log string) then we would
// need to add a critical section to protect access to the shared data.
class FailedMessageLog
{
public:

    // This handler is a 'blind' handler which takes the unhandled message as raw data.
    inline void Handle(const void *const data, const Theron::uint32_t size, const Theron::Address from)
    {
        printf("Unhandled message of %d bytes sent from address %d:\n", size, from.AsInteger());

        // Dump the message data as hex words.
        if (data)
        {
            const char *const format("[%d] 0x%08x\n");

            const unsigned int *const begin(reinterpret_cast<const unsigned int *>(data));
            const unsigned int *const end(begin + size / sizeof(unsigned int));

            for (const unsigned int *word(begin); word != end; ++word)
            {
                printf(format, word - begin, *word);
            }
        }
    }
};


int main()
{
    Theron::Framework framework;
    Theron::Receiver receiver;
    
    // Register the custom fallback handler with the framework.
    // This handler is executed for any messages that either aren't delivered
    // or aren't handled by the actors to which they are delivered.
    // The default fallback handler asserts, which is useful but not so pretty
    // in an example. So here we show how to register a custom handler.
    FailedMessageLog log;
    framework.SetFallbackHandler(&log, &FailedMessageLog::Handle);
    
    // Create an actor and send some messages to it, which it doesn't handle.
    // The messages are delivered but not handled by the actor, so are
    // caught by the log registered as the framework's default fallback message handler.
    Theron::ActorRef actor(framework.CreateActor<Actor>());

    printf("Sending message (16384, 1.5f) to actor\n");
    framework.Send(Message(16384, 1.5f), receiver.GetAddress(), actor.GetAddress());

    printf("Sending message (507, 2.0f) to actor\n");
    framework.Send(Message(507, 2.0f), receiver.GetAddress(), actor.GetAddress());

    return 0;
}

