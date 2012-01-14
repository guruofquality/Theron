// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


//
// This sample shows how to use message registration to avoid the use of Runtime
// Type Information (RTTI). By explicitly registering the message types used in
// an application, we cause Theron to use its own internal RTTI system, using type
// IDs explicitly associated with messages during sending. This in turn allows us
// to turn off the built-in C++ RTTI, usually by means of a compiler option.
// Doing so can result in a storage saving as it removes the implicitly added
// runtime type information added to all types by the compiler. Removing this
// implicit storage can be important in embedded environments such as games consoles,
// where a type needs to be exactly its raw size with no extra storage overhead.
// Note that this sample can be built with C++ RTTI turned off, whereas the other
// samples generally can't.
//


#include <stdio.h>


// Enable checking for unregistered message types.
#define THERON_ENABLE_MESSAGE_REGISTRATION_CHECKS 1


#include <Theron/Actor.h>
#include <Theron/Address.h>
#include <Theron/Framework.h>
#include <Theron/Receiver.h>
#include <Theron/Register.h>


namespace Example
{


// Two different message types
struct FloatMessage
{
    explicit FloatMessage(const float value) : mValue(value) { }
    float mValue;
};

struct IntegerMessage
{
    explicit IntegerMessage(const int value) : mValue(value) { }
    int mValue;
};


// A simple actor that just receives messages and sends them back.
class SimpleActor : public Theron::Actor
{
public:

    inline SimpleActor()
    {
        // Register message handler functions.
        RegisterHandler(this, &SimpleActor::FloatHandler);
        RegisterHandler(this, &SimpleActor::IntegerHandler);
    }

private:

    inline void FloatHandler(const FloatMessage &message, const Theron::Address from)
    {
        printf("SimpleActor received FloatMessage with contents '%f'\n", message.mValue);
        TailSend(message, from);
    }

    inline void IntegerHandler(const IntegerMessage &message, const Theron::Address from)
    {
        printf("SimpleActor received IntegerMessage with contents '%d'\n", message.mValue);
        TailSend(message, from);
    }
};


} // namespace Example


// Register the message types. This turns off the use of dynamic_cast within Theron,
// which in turn allows us to disable the built-in C++ Runtime Type Information (RTTI).
// Note that message registration is entirely optional, and should only by used if
// you specifically need to build your application with the built-in C++ RTTI disabled.
// If message type registration is used then it must be used for all messages in the
// application: failure to register a message type will result in errors.
// An important thing to note is that THERON_REGISTER_MESSAGE can only be used within the
// global namespace -- so we can't register messages immediately after their declarations,
// if the message declarations are inside namespaces.
THERON_REGISTER_MESSAGE(Example::FloatMessage);
THERON_REGISTER_MESSAGE(Example::IntegerMessage);


int main()
{
    Theron::Framework framework;
    Theron::ActorRef actor(framework.CreateActor<Example::SimpleActor>());
    Theron::Receiver receiver;

    // Send the actor one message of each kind.
    if (!framework.Send(Example::FloatMessage(5.0f), receiver.GetAddress(), actor.GetAddress()))
    {
        printf("Failed to send message!\n");
    }

    if (!framework.Send(Example::IntegerMessage(6), receiver.GetAddress(), actor.GetAddress()))
    {
        printf("Failed to send message!\n");
    }

    // Wait for both reply messages before terminating.
    receiver.Wait();
    receiver.Wait();

    printf("Received two reply messages\n");
    return 0;
}

