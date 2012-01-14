// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


//
// This sample demonstrates the use of abstract data types as message values.
// The noteworthy thing about this is that these value types allocate memory
// internally and have non-trivial copy-construction and assignment, and
// hence non-trivial message sending. The object is copied during message
// passing, so that the recipient of the message sees a new copy that is
// identical to the original owned by the sender. The copy exists in a piece
// of memory internal to Theron, and is copy-constructed using the copy-constructor
// of the message type (which must be copyable). This is similar to, for example,
// the copying of value types in STL containers.
//


#include <vector>
#include <stdio.h>

#include <Theron/Framework.h>
#include <Theron/Receiver.h>
#include <Theron/Actor.h>


// A non-trivial (but still copyable) message type.
typedef std::vector<int> IntegerVector;


// A simple actor that catches messages and prints out their contents.
class Catcher : public Theron::Actor
{
public:

    inline Catcher()
    {
        RegisterHandler(this, &Catcher::Handler);
    }

private:

    inline void Handler(const IntegerVector &message, const Theron::Address from)
    {
        const size_t numValues(message.size());
        const char *delimiter = ":";

        printf("Received message with %d values", numValues);
        for (size_t index = 0; index < numValues; ++index)
        {
            printf("%s %d", delimiter, message[index]);
            delimiter = ",";
        }

        printf("\n");

        // Send a dummy message back for synchronization.
        Send(true, from);
    }
};


int main()
{
    Theron::Framework framework;
    Theron::ActorRef actor(framework.CreateActor<Catcher>());

    // Create a message and fill it with some values.
    IntegerVector message;
    message.push_back(4);
    message.push_back(7);
    message.push_back(2);

    // Send the message to the catcher, passing the address of a local receiver
    // as the 'from' address. Note that the message is copied during message
    // passing, including all of its contents. See the EnvelopeMessages sample
    // for a workaround that avoids this overhead.
    Theron::Receiver receiver;
    framework.Send(message, receiver.GetAddress(), actor.GetAddress());

    // Wait for confirmation that the message was received before terminating.
    receiver.Wait();

    return 0;
}

