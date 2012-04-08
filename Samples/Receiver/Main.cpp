// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


//
// This sample shows how to use a Receiver to handle messages in non-actor code.
//


#include <stdio.h>
#include <vector>

#include <Theron/Framework.h>
#include <Theron/Receiver.h>
#include <Theron/Actor.h>


// The message type accepted by the actor, which contains an integer value.
struct Message
{
    inline Message() : mValue(0)
    {
    }

    inline explicit Message(const int value) : mValue(value)
    {
    }
    
    int mValue;
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


// An example of a non-actor class that can handle messages received by a Receiver.
// All a class needs, to be able to handle messages, is a public handler method
// accepting a from address and a message value of the correct type.
class MessageCollector
{
public:

    // Handler function that we register with the Receiver.
    inline void Handler(const Message &message, const Theron::Address /*from*/)
    {
        // Collect the message.
        mMessages.push_back(message);
    }

    inline const std::vector<Message> &GetMessages() const
    {
        return mMessages;
    }

private:

    std::vector<Message> mMessages;
};


int main()
{
    // Create a framework and an actor object.
    Theron::Framework framework;
    Theron::ActorRef responder(framework.CreateActor<ResponderActor>());

    // Create a message receiver. This is an entity with a unique address,
    // capable of receiving messages. Instantiating one in non-actor code
    // allows that code to receive messages, acting like an actor from the
    // point of view of any actors in the application.    
    Theron::Receiver receiver;

    // Create a message collector to hold the message received by the receiver,
    // and register its handler method with the receiver.
    // The handler is registered for the specific message type that it accepts,
    // and will only be called on receipt of messages of that type. The message
    // collector we're using in this example is just a simple class with a message
    // handler method capable of handling messages of the correct type. Any such
    // method of any class can be registered as a handler for incoming messages
    // with a Receiver. Any number of handlers can be registered, for any number
    // of different message types, and multiple handlers can be registered for the
    // same message type. This handler registration facility is optional, and allows
    // non-actor code to execute callbacks when messages are received. If no handler
    // is registered then the calling code can still Wait() on the arrival of a
    // message, but it can't accept the message or inspect it.
    MessageCollector messageCollector;
    receiver.RegisterHandler(&messageCollector, &MessageCollector::Handler);

    // Send a message to the actor using the unique address of the receiver as the
    // 'from' address. The responder actor sends the message back to that address.
    const Theron::Address fromAddress(receiver.GetAddress());
    responder.Push(Message(5), fromAddress);

    // Send a second message to the actor with a different value.
    responder.Push(Message(6), fromAddress);

    // Wait for indication that the first message has arrived at the receiver.
    // This also guarantees that any handlers registered with the receiver
    // for that message type will have been executed before returning,
    // allowing synchronization of the calling code with the message handling.
    // We effectively go to sleep and get woken up when a message arrives.
    // Note that we get woken up when any message arrives - even if it was of
    // a type for which no handlers were registered. Note also that in practice
    // the message may have arrived *before* we call Wait() - in which case the
    // call to Wait() will return immediately.
    receiver.Wait();

    // Check the value of the message received and stored in the collector.
    // It's safe to do this because the call to Wait() above will only return
    // once the handler has executed completely.
    printf("Received first message with value '%d'\n", messageCollector.GetMessages()[0].mValue);

    // Wait for the second message to arrive and be handled.
    receiver.Wait();

    // Check the value of the new message.
    printf("Received second message with value '%d'\n", messageCollector.GetMessages()[1].mValue);
    
    return 0;
}

