// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_ACTORREF_H
#define THERON_ACTORREF_H


/**
\file ActorRef.h
Actor reference, references an actor in user code.
*/


#include <Theron/Detail/Debug/Assert.h>
#include <Theron/Detail/Messages/MessageSender.h>
#include <Theron/Detail/ThreadPool/WorkerContext.h>

#include <Theron/Actor.h>
#include <Theron/Address.h>
#include <Theron/AllocatorManager.h>
#include <Theron/Defines.h>
#include <Theron/IAllocator.h>


namespace Theron
{


class Framework;


/**
\brief Used to reference an actor in user code.

\ref Actor "Actors" can't be created or referenced directly in user code.
Instead, when an actor is created with \ref Framework::CreateActor,
an ActorRef is returned referencing the created actor.
ActorRef objects are like reference-counted smart pointers to actors,
and expose only appropriate actor methods to the code that owns the
reference.

ActorRef objects can be copied, assigned, passed by value or by reference to
functions, and returned as function return values. When an ActorRef is copied
or assigned, the actor it references is not copied, and remains unchanged.
Only the reference is copied.

Internally, \ref Actor "Actors" are reference counted, and automatically destroyed
when they become unreferenced. The number of ActorRef objects referencing each actor is
tracked, and updated whenever an ActorRef is constructed, copied, assigned,
or destructed. When the last ActorRef referencing an \ref Actor is destructed,
the Actor's reference count drops to zero, indicating that the actor has become
unreferenced. Unreferenced actors are scheduled for destruction, and will be
destroyed by a worker thread after some short period. See the \ref ActorRef
destructor documentation for more information.

\note Although actor references can be safely copied and assigned, copying actor
references is not completely cost-free, and imposes some overhead due to the
internal threadsafe reference counting used to detect unreferenced actors.
Therefore it's more efficient to reference an existing ActorRef instance, where
possible, rather than to copy it. For example, it's generally faster to pass actor
references to functions by reference rather than by value.

\see <a href="http://www.theron-library.com/index.php?t=page&p=UsingActorReferences">Using actor references</a>
*/
class ActorRef
{

public:

    friend class Framework;

    /**
    \brief Static method that returns a null actor reference.

    The null actor reference doesn't reference any actor and is guaranteed
    not to be equal to any non-null actor reference.

    \code
    class Actor : public Theron::Actor
    {
    };

    Theron::Framework framework;
    Theron::ActorRef actor(framework.CreateActor<Actor>());

    assert(actor != ActorRef::Null(), "Failed to create actor!");
    \endcode
    */
    inline static ActorRef Null()
    {
        return ActorRef();
    }

    /**
    \brief Default constructor.

    Constructs a null actor reference, referencing no actor.

    \code
    Theron::ActorRef actor;
    assert(actor == ActorRef::Null(), "Expected default ActorRef to be null");
    \endcode
    */
    inline ActorRef();

    /**
    \brief Copy constructor.

    Copies an actor reference, constructing another actor reference referencing
    the same actor as the first.

    \code
    class Actor : public Theron::Actor
    {
    };

    Theron::Framework framework;
    Theron::ActorRef actorOne(framework.CreateActor<Actor>());
    Theron::ActorRef actorTwo(actorOne);

    assert(actorOne == actorTwo, "Expected actor references to be equal");
    \endcode

    Although ActorRef objects are relatively lightweight, it is still faster to
    reference an existing ActorRef than to copy it. For example, this style is preferable:

    \code
    void ProcessActor(ActorRef &actor)
    {
        /// Do stuff
    }
    \endcode

    to this style:

    \code
    void ProcessActor(ActorRef actor)
    {
        /// Do stuff
    }
    \endcode

    */
    inline ActorRef(const ActorRef &other);

    /**
    \brief Assignment operator.

    Sets this actor reference to reference the same actor as another.
    After assignment the actor previously referenced by this ActorRef, if any, is
    no longer referenced, and will be garbage collected if it has become completely
    unreferenced.

    Although ActorRef objects are relatively lightweight, it is still faster to
    reference an existing ActorRef than to assign it to another copy.
    */
    inline ActorRef &operator=(const ActorRef &other);

    /**
    \brief Destructor.

    Destroys a reference to an actor.

    Actors are automatically destructed by a process called \em garbage \em collection
    when all of the ActorRef objects that reference them have been destructed.
    The number of ActorRef objects referencing each actor is counted, and when the
    count reaches zero on destruction of the last referencing ActorRef, the actor is
    scheduled for deletion. The actual destruction of the dereferenced actor is 
    asynchronous, and isn't guaranteed to happen immediately. However it's usually
    almost immediate, and the actor will always be destroyed before destruction
    of the Framework object that owned it.

    \code
    class Actor : public Theron::Actor
    {
    };

    int main()
    {
        // Framework constructed.
        Theron::Framework framework;

        // Actor created and referenced by ActorRef.
        Theron::ActorRef actor(framework.CreateActor<Actor>());

        // Actor used
        // ...

        // ActorRef destructed, actor becomes unreferenced and is garbage collected.
        // Framework destructed after destroying the actor.
    }
    \endcode

    \note An important requirement is that \ref Framework objects must always
    outlive all \ref ActorRef objects created with them. This ensures that the
    actors created within the framework become dereferenced, and so are garbage
    collected, prior to the destruction of the \ref Framework itself.
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
    inline const Address &GetAddress() const;

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
    this method will still return true. This surprising behaviour is a result
    of the asynchronous nature of message sending: the sender doesn't wait
    for the recipient to process the message. It is the sender's
    responsibility to ensure that messages are appropriate for the actors to
    which they are sent. Actor implementations can also register a default
    message handler (see \ref Actor::SetDefaultHandler).
    */
    template <class ValueType>
    inline bool Push(const ValueType &value, const Address &from);

    /**
    \brief Gets the number of messages queued at this actor, awaiting processing.

    Returns the number of messages currently in the message queue of the referenced actor.
    The messages in the queue are those that have been received by the actor but for
    which registered message handlers have not yet been executed, and so are still
    awaiting processing.
    
    \note If the referenced actor is being processed by a worker thread at the time of
    the call, the returned count doesn't include the message whose receipt triggered the
    processing.
    */
    inline uint32_t GetNumQueuedMessages() const;

private:

    /// Constructor. Constructs a reference to the given actor.
    /// \param actor A pointer to the actor to be referenced.
    /// \note This method is private and is accessed only by the Framework class.
    inline explicit ActorRef(Actor *const actor);

    /// References the actor referenced by the actor reference.
    inline void Reference();

    /// Dereferences the actor previously referenced by the actor reference.
    inline void Dereference();

    /// Gets a pointer to the pulse counter of the framework that owns the referenced actor.
    uint32_t *GetPulseCounterAddress() const;

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


THERON_FORCEINLINE const Address &ActorRef::GetAddress() const
{
    THERON_ASSERT(mActor);
    return mActor->GetAddress();
}


THERON_FORCEINLINE void ActorRef::Reference()
{
    if (mActor)
    {
        mActor->Reference();
    }
}


THERON_FORCEINLINE void ActorRef::Dereference()
{
    if (mActor)
    {
        mActor->Dereference();
    }
}


template <class ValueType>
THERON_FORCEINLINE bool ActorRef::Push(const ValueType &value, const Address &from)
{
    IAllocator *const messageAllocator(AllocatorManager::Instance().GetAllocator());
    Framework *const framework(&mActor->GetFramework());
    const Address actorAddress(mActor->GetAddress());
    uint32_t *const pulseCounterAddress(GetPulseCounterAddress());

    return Detail::MessageSender::Send(
        messageAllocator,
        pulseCounterAddress,
        framework,
        value,
        from,
        actorAddress);
}


THERON_FORCEINLINE uint32_t ActorRef::GetNumQueuedMessages() const
{
    return mActor->GetNumQueuedMessages();
}


} // namespace Theron


#endif // THERON_ACTORREF_H

