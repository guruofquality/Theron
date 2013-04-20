// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_ACTORREF_H
#define THERON_ACTORREF_H


/**
\file ActorRef.h
Legacy support for actor references, which were deprecated in version 4.0.
*/


#include <Theron/Address.h>
#include <Theron/Assert.h>
#include <Theron/Defines.h>

#include <Theron/Detail/Messages/IMessage.h>
#include <Theron/Detail/Messages/MessageCreator.h>
#include <Theron/Detail/Messages/MessageSender.h>
#include <Theron/Detail/Scheduler/MailboxContext.h>


namespace Theron
{


class Actor;
class Framework;


/**
\brief Legacy utility for backwards compatibility.

\note In versions of Theron from 4.0 onwards, there is no need to use ActorRef
objects at all. They are provided only for backwards compatibility.

In versions of Theron prior to 4.0, actors couldn't be constructed directly
in user code. Instead you had to ask a Framework to create one for you, using
the Framework::CreateActor method template. Instead of returning the actor itself,
CreateActor returned a <i>reference</i> to the actor in the form of an ActorRef
object.

ActorRefs were lightweight, copyable references to actors. When an ActorRef was
copied (or passed or returned by value) the ActorRef was copied, but the copy
still referenced the same Actor, which was not itself copied. The Actor itself
was reference-counted, and was scheduled for garbage collection when the last
ActorRef referencing it was destructed, and it became unreferenced.

\code
// LEGACY CODE!
class MyActor : public Theron::Actor
{
};

int main()
{
    Theron::Framework framework;
    Theron::ActorRef actorRef(framework.CreateActor<MyActor>());

    // (1) The actorRef goes out of scope and destructed.
    // (2) Actor becomes unreferenced and is scheduled for garbage collection.
    // (3) Framework worker threads garbage collect the unreferenced actor.
    // (4) The framework goes out of scope and is destructed.
}
\endcode

In versions of Theron starting with 4.0, Actors are first-class citizens and
behave like vanilla C++ objects. They can be constructed directly with no
call to Framework::CreateActor. Once constructed they are referenced directly
by user code with no need for ActorRef proxy objects. Finally, like traditional
C++ objects, once they go out of scope or are deleted they are destructed
immediately rather than in a deferred fashion by a separate garbage collection
process.

When writing new code, follow the new, simpler construction pattern where actors
are constructed directly and not referenced by ActorRefs:

\code
// New code
class MyActor : public Theron::Actor
{
public:

    MyActor(Theron::Framework &framework) : Theron::Actor(framework)
    {
    }
};

int main()
{
    Theron::Framework framework;
    MyActor actor(framework);

    // (1) The actor goes out of scope and destructed.
    // (4) the Framework goes out of scope and is destructed.
}
\endcode
*/
class ActorRef
{

public:

    friend class Framework;

    /**
    \brief Static method that returns a null actor reference.

    The null actor reference doesn't reference any actor and is guaranteed
    not to be equal to any non-null actor reference.
    */
    inline static ActorRef Null()
    {
        return ActorRef();
    }

    /**
    \brief Default constructor.

    Constructs a null actor reference, referencing no actor.
    */
    inline ActorRef();

    /**
    \brief Copy constructor.

    Copies an actor reference, constructing another actor reference referencing
    the same actor as the first.
    */
    inline ActorRef(const ActorRef &other);

    /**
    \brief Assignment operator.

    Sets this actor reference to reference the same actor as another.
    After assignment the actor previously referenced by this ActorRef, if any, is
    no longer referenced, and will be garbage collected if it has become completely
    unreferenced.
    */
    inline ActorRef &operator=(const ActorRef &other);

    /**
    \brief Destructor.

    Destroys a reference to an actor.

    \note An important requirement is that \ref Framework objects must always
    outlive all \ref ActorRef objects created with them. This ensures that the
    actors created within the framework become dereferenced, and so are destructed,
    prior to the destruction of the Framework itself.
    */
    inline ~ActorRef();

    /**
    \brief Equality operator.

    Returns true if the given actor reference references the same actor as
    this actor reference does, or if both are null.
    */
    inline bool operator==(const ActorRef &other) const;

    /**
    \brief Inequality operator.

    Returns true if the given actor reference references a different actor
    from the one referenced by this actor reference, or if one is null and
    the other is not.
    */
    inline bool operator!=(const ActorRef &other) const;

    /**
    \brief Returns the unique address of the referenced actor.

    \return The unique address of the actor.
    */
    Address GetAddress() const;

    /**
    \brief Pushes a message into the referenced actor.

    This method is an alternative to \ref Framework::Send, which is the more
    conventional and general way to send messages to actors. \ref Push can be
    called in situations where the caller happens to have an ActorRef referencing
    the actor they want to message.

    \tparam ValueType The message type (any copyable class or Plain Old Datatype).
    \return True, if the actor accepted the message.

    \note The return value of this method should be understood to mean
    just that the message was *accepted* by the actor. This doesn't mean
    necessarily that the actor took any action in response to the message.
    If the actor has no handlers registered for messages of that type, then
    the message will simply be consumed without any effect. In such cases
    this method will still return true. This surprising behavior is a result
    of the asynchronous nature of message sending: the sender doesn't wait
    for the recipient to process the message. It is the sender's
    responsibility to ensure that messages are appropriate for the actors to
    which they are sent. Actor implementations can also register a default
    message handler (see \ref Actor::SetDefaultHandler).
    */
    template <class ValueType>
    inline bool Push(const ValueType &value, const Address &from);

    /**
    \brief Gets the number of messages queued at the referenced actor.

    Returns the number of messages currently in the message queue of the actor.
    The messages in the queue are those that have been received by the actor but
    for which registered message handlers have not yet been executed, and so are
    still awaiting processing.
    */
    uint32_t GetNumQueuedMessages() const;

private:

    /// Constructor. Constructs a reference to the given actor.
    /// \param actor A pointer to the actor to be referenced.
    /// \note This method is private and is accessed only by the Framework class.
    inline explicit ActorRef(Actor *const actor);

    /// References the actor referenced by the actor reference.
    void Reference();

    /// Dereferences the actor previously referenced by the actor reference.
    void Dereference();

    Detail::MailboxContext *GetMailboxContext();
    uint32_t GetFrameworkIndex() const;

    Actor *mActor;      ///< Pointer to the referenced actor.
};


THERON_FORCEINLINE ActorRef::ActorRef() : mActor(0)
{
}


THERON_FORCEINLINE ActorRef::ActorRef(Actor *const actor) : mActor(actor)
{
    Reference();
}


THERON_FORCEINLINE ActorRef::ActorRef(const ActorRef &other) : mActor(other.mActor)
{
    Reference();
}


THERON_FORCEINLINE ActorRef &ActorRef::operator=(const ActorRef &other)
{
    Dereference();
    mActor = other.mActor;
    Reference();
    
    return *this;
}


THERON_FORCEINLINE ActorRef::~ActorRef()
{
    Dereference();
}


THERON_FORCEINLINE bool ActorRef::operator==(const ActorRef &other) const
{
    return (mActor == other.mActor);
}


THERON_FORCEINLINE bool ActorRef::operator!=(const ActorRef &other) const
{
    return (mActor != other.mActor);
}


template <class ValueType>
THERON_FORCEINLINE bool ActorRef::Push(const ValueType &value, const Address &from)
{
    // Use the per-framework context, which is shared between threads.
    Detail::MailboxContext *const mailboxContext(GetMailboxContext());

    // Allocate a message. It'll be deleted by the worker thread that handles it.
    Detail::IMessage *const message(Detail::MessageCreator::Create(
        mailboxContext->mMessageAllocator,
        value,
        from));

    if (message == 0)
    {
        return false;
    }

    // Call the message sending implementation using the acquired processor context.
    // Send the message to the actor via the usual path instead of trying to be sneaky.
    return Detail::MessageSender::Send(
        0,
        mailboxContext,
        GetFrameworkIndex(),
        message,
        GetAddress());
}


} // namespace Theron


#endif // THERON_ACTORREF_H
