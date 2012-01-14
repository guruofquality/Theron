// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


//
// This sample shows how to use derivation to create subclasses of messages.
//


#include <stdio.h>

#include <Theron/Actor.h>
#include <Theron/Framework.h>
#include <Theron/Receiver.h>


// A base message type.
class BaseMessage
{
public:

    inline explicit BaseMessage(const int value) : mBaseValue(value)
    {
    }

    int mBaseValue;
};


// A message type derived from the base type.
// This is-a relation means that Message is also of type BaseMessage.
class DerivedMessage : public BaseMessage
{
public:

    inline DerivedMessage(const int baseValue, const int derivedValue) :
      BaseMessage(baseValue),
      mDerivedValue(derivedValue)
    {
    }

    int mDerivedValue;
};


// A simple actor that accepts messages of two related types.
class SimpleActor : public Theron::Actor
{
public:

    inline SimpleActor()
    {
        RegisterHandler(this, &SimpleActor::BaseMessageHandler);
        RegisterHandler(this, &SimpleActor::DerivedMessageHandler);
    }

private:

    // Handler for messages of type BaseMessage.
    inline void BaseMessageHandler(const BaseMessage &message, const Theron::Address from)
    {
        Send(message, from);
        printf("Received BaseMessage with value '%d'\n", message.mBaseValue);
    }

    // Handler for messages of type DerivedMessage.
    inline void DerivedMessageHandler(const DerivedMessage &message, const Theron::Address from)
    {
        Send(message, from);
        printf("Received DerivedMessage with base value '%d', derived value '%d'\n",
            message.mBaseValue,
            message.mDerivedValue);
    }
};


int main()
{
    Theron::Framework framework;
    Theron::ActorRef simpleActor(framework.CreateActor<SimpleActor>());

    Theron::Receiver receiver;
    
    // Send two messages, one a base message and the other a derived message.
    // By virtue of derivation both messages are valid BaseMessage messages.
    // Additionally the derived message is a valid DerivedMessage message.
    simpleActor.Push(BaseMessage(5), receiver.GetAddress());
    simpleActor.Push(DerivedMessage(6, 10), receiver.GetAddress());

    // We might have expected three messages in response, with the DerivedMessage
    // triggering both BaseMessageHandler and DerivedMessageHandler. But in
    // practice currently DerivedMessage triggers only DerivedMessageHandler.
    // This somewhat unexpected behaviour is due to an implementation limitation.
    // Basically we don't know how to do it :)
    receiver.Wait();
    receiver.Wait();

    return 0;
}

