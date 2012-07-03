// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


//
// This sample demonstrates the use of 'envelope' (or 'proxy') messages that are
// just lightweight references to owned objects. The message is copyable, but the
// referenced object is not copied when the message is copied. Instead just the reference
// is copied. This allows heavyweight objects, whose copying is expensive, to be safely
// sent in messages without introducing shared memory, by senging lightweight references
// to them instead.
//
// Additionally, the envelope message imposes the single ownership of the referenced object
// (and the uniqueness of the reference), such that only one of the copies of the envelope
// actually contains a valid reference to the 'letter' object at any time. This ensures that
// once the sender has sent a message, the recipient can access it but the sender can not.
// This feature is important for preserving the semantics of the actor model. If we
// simply sent a raw pointer to the referenced object as a message instead, there would be
// nothing stopping the sender from dereferencing the pointer after sending it, introducing
// the possibility of shared memory (and related thread synchronization issues that the
// Actor Model, if used correctly, solves). The simplest way to prevent this is to always
// send actual copies of objects in messages, allowing the entire object to be deep-copied
// in memory. However this can be expensive, for heavyweight objects. Instead, envelope
// messages provide the semantics of single-ownership without the copying overhead.
//
// Note that the envelope class template provided here is a simplified implementation and
// shows just the basics (although it works well enough, for objects that don't require
// specialized construction). It is similar in concept to well-known standard idioms such
// as smart pointers (eg. auto_ptr and unique_ptr), which are more general and better
// developed. Use one of those implementations instead (for example the ones in Boost),
// if they suit your application. The point of this example is to explain the concept.
//


#include <vector>
#include <stdio.h>
#include <assert.h>

#include <Theron/Framework.h>
#include <Theron/Receiver.h>
#include <Theron/Actor.h>


// An envelope message class template that acts as a lightweight reference to an owned object.
// The message type itself is just an 'envelope' that contains the real message value, or 'letter'.
template <class ObjectType>
class EnvelopeMessage
{
public:

    inline EnvelopeMessage() : m_object(new ObjectType())
    {
    }

    inline ~EnvelopeMessage()
    {
        if (m_object)
        {
            delete m_object;
        }
    }

    // Copy-constructor, allowing the envelope to be copied and so sent as a message.
    // Note that the referenced object itself is not copied; just the reference to it.
    inline EnvelopeMessage(const EnvelopeMessage &other) : m_object(other.m_object)
    {
        // Forcibly invalidate the source object's reference to the referenced object.
        // This ensures that only one valid reference to the object exists at any time.
        const_cast<EnvelopeMessage *>(&other)->m_object = 0;
    }

    // Assignment operator, allowing the envelope to be assigned.
    // Note that the referenced object itself is not copied; just the reference to it.
    // Theron currently uses the class copy-constructor to copy message types, but it's
    // good practice to also provide an assignment operator, which may be needed by other code.
    inline EnvelopeMessage &operator=(const EnvelopeMessage &other)
    {
        // Grab the referenced object and invalidate the source object's reference to it.
        m_object = other.m_object;
        const_cast<EnvelopeMessage *>(&other)->m_object = 0;
    }

    // Calling code calls this to check whether the envelope still references the object.
    inline bool Valid() const
    {
        return (m_object != 0);
    }

    // Calling code calls this to access the referenced object, or 'letter'.
    // This function will assert in debug builds if the envelope no longer references the letter.
    inline const ObjectType &Object() const
    {
        assert(Valid());
        return *m_object;
    }

    inline ObjectType &Object()
    {
        assert(Valid());
        return *m_object;
    }

private:

    ObjectType *m_object;
};


// A non-trivial (but still copyable) message value type.
typedef std::vector<int> IntegerVector;

// Instantiation of the envelope class template for the integer vector type.
typedef EnvelopeMessage<IntegerVector> IntegerVectorEnvelope;


// A simple actor that catches messages and prints out their contents.
class Catcher : public Theron::Actor
{
public:

    inline Catcher()
    {
        RegisterHandler(this, &Catcher::Handler);
    }

private:

    inline void Handler(const IntegerVectorEnvelope &envelope, const Theron::Address from)
    {
        assert(envelope.Valid());
        const IntegerVector &contents(envelope.Object());

        const size_t numValues(contents.size());
        const char *delimiter = ":";

        printf("Received message with %d values", static_cast<int>(numValues));
        for (size_t index = 0; index < numValues; ++index)
        {
            printf("%s %d", delimiter, contents[index]);
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

    // Create an envelope message and fill its owned vector with some values.
    IntegerVectorEnvelope envelope;
    envelope.Object().push_back(4);
    envelope.Object().push_back(7);
    envelope.Object().push_back(2);

    // Send the message to the catcher, passing the address of a local receiver
    // as the 'from' address.
    Theron::Receiver receiver;
    framework.Send(envelope, receiver.GetAddress(), actor.GetAddress());

    // Note that the envelope owned by the sender no longer contains the 'letter';
    // the reference to it has been passed to the recipient and is no longer accessible here.
    // A call to envelope.Object() here would be caught by an assert, in debug builds.
    printf("Sender %s has access to sent object\n", envelope.Valid() ? "still" : "no longer");

    // Wait for confirmation that the message was received before terminating.
    receiver.Wait();

    return 0;
}

