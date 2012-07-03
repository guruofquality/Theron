// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_ACTOR_H
#define THERON_ACTOR_H


/**
\file Actor.h
Actor baseclass.
*/


#include <new>

#include <Theron/BasicTypes.h>
#include <Theron/Address.h>
#include <Theron/AllocatorManager.h>
#include <Theron/Defines.h>
#include <Theron/IAllocator.h>

#include <Theron/Detail/Containers/IntrusiveList.h>
#include <Theron/Detail/Core/ActorCore.h>
#include <Theron/Detail/Core/ActorCreator.h>
#include <Theron/Detail/Debug/Assert.h>
#include <Theron/Detail/Handlers/BlindDefaultHandler.h>
#include <Theron/Detail/Handlers/DefaultHandler.h>
#include <Theron/Detail/Handlers/IDefaultHandler.h>
#include <Theron/Detail/Handlers/MessageHandler.h>
#include <Theron/Detail/Handlers/IMessageHandler.h>
#include <Theron/Detail/Handlers/MessageHandlerCast.h>
#include <Theron/Detail/Messages/MessageSender.h>
#include <Theron/Detail/Messages/MessageTraits.h>
#include <Theron/Detail/Threading/Lock.h>
#include <Theron/Detail/ThreadPool/WorkerContext.h>


/**
Main namespace, containing all public API components.
*/
namespace Theron
{


namespace Detail
{
class ActorDestroyer;
}


/**
\brief The actor baseclass.

All actors in Theron must derive from this class.
It provides the core functionality of an actor, such as the ability
to \ref RegisterHandler "register message handlers" and \ref Send "send messages",
including responding to messages received from other actors. When implementing
actors derived from this baseclass, users can call the various protected methods
of the baseclass to perform actions like registering message handlers and sending
messages.

Actors classes, derived from the Actor baseclass, can't be constructed directly
in user code. Instead, they are created within an owning \ref Framework, which
is responsible for allocating, constructing, executing and destroying the actor.

Actor objects are reference counted, and are automatically destroyed when they become
unreferenced. An actor becomes unreferenced when the last reference to it
is destructed. See \ref ActorRef for more information on actor garbage collection.

The maximum number of actors that can be created in an application is limited
by the \ref THERON_MAX_ACTORS define, which defines the number of unique
actor addresses.

\see <a href="http://www.theron-library.com/index.php?t=page&p=CreatingAnActor">Creating an Actor</a>
\see <a href="http://www.theron-library.com/index.php?t=page&p=InitializingAnActor">Initializing an Actor</a>
\see <a href="http://www.theron-library.com/index.php?t=page&p=UsingActorReferences">Using actor references</a>
\see <a href="http://www.theron-library.com/index.php?t=page&p=AligningActors">Aligning actors</a>
*/
class Actor
{

public:

    friend class Detail::ActorCore;
    friend class Detail::ActorCreator;
    friend class Detail::ActorDestroyer;
    friend class ActorRef;

    /**
    \brief Default constructor.

    Actor classes can be instantiated using the \ref CreateActor methods of
    a \ref Framework object, which returns an \ref ActorRef object referencing the
    created actor. The created actor can't be accessed directly. Instead it is
    referenced in user code by \ref ActorRef objects, which are copyable, lightweight
    references to the actor. ActorRef objects are like conventional references and
    can be copied, assigned, passed by value or reference, and returned by value from
    functions.

    \code
    class MyActor : public Theron::Actor
    {
    };

    Theron::Framework framework;
    Theron::ActorRef myActor = framework.CreateActor<MyActor>();
    \endcode

    A powerful feature of actors is that they can create other actors, allowing
    the building of complex subsystems of actors using object-like abstraction.
    See the documentation of the \ref GetFramework method for more information.

    \note Only the \ref Framework can construct actors.
    */
    inline Actor();

    /**
    \brief Baseclass virtual destructor.

    Just as actors can't be instantiated directly in user code, but instead are
    instantiated within an owning \ref Framework, they also aren't destructed
    directly in user code. Instead they are reference counted and destroyed
    automatically by a process called garbage collection. See the documentation
    of the \ref ActorRef class for more information.

    When an actor is garbage collected, the destructor of the derived actor class
    is called, allowing the actor to perform whatever cleanup it needs to do.
    */
    inline virtual ~Actor();

protected:

    /**
    \brief Returns the unique address of the actor.

    This enables an actor to determine its own address and, for example,
    to send that address to another actor in a message. In this example
    we print out the address ("identity") of the actor in response to a query:

    \code
    class Actor : public Theron::Actor
    {
    public:

        struct IdentifyMessage
        {
        };

        Actor()
        {
            RegisterHandler(this, &Actor::Identify);
        }

    private:

        inline void Identify(const IdentifyMessage &message, const Theron::Address from)
        {
            printf("Actor address is: %d\n", GetAddress().AsInteger());
        }
    };
    \endcode

    \note This method can safely be called inside an actor message handler,
    constructor, or destructor.
    */
    inline const Address &GetAddress() const;

    /**
    \brief Returns a reference to the framework that owns the actor.

    This enables an actor to access the framework that owns it. One reason
    this may be useful is to allow the actor to create other actors, which
    effectively become nested inside it as its children. By nesting actors
    it's possible to build complex subsystems, abstracted away behind the
    message interface of a single actor.

    To create actors that are 'children' of another actor, the parent actor uses the
    protected \ref GetFramework method to retrieve a pointer to the Framework within
    which it itself was created, then creates the child actors using the Framework's
    \ref Framework::CreateActor "CreateActor" methods.

    By holding on to the \ref ActorRef objects returned by \ref Framework::CreateActor
    "CreateActor" as member variables, actors can ensure their child actors are kept
    alive, and not garbage collected. Conversely, when the parent actor becomes
    unreferenced and is destroyed by garbage collection, the ActorRef objects it
    holds as members are destructed with it, causing the children to be cleanly garbage
    collected too.

    \code
    class Wheel : public Theron::Actor
    {
    };

    class Car : public Theron::Actor
    {
    public:

        Car()
        {
            mWheels[0] = GetFramework().CreateActor<Wheel>();
            mWheels[1] = GetFramework().CreateActor<Wheel>();
            mWheels[2] = GetFramework().CreateActor<Wheel>();
            mWheels[3] = GetFramework().CreateActor<Wheel>();
        }

    private:

        Theron::ActorRef mWheels[4];
    };
    \endcode

    \note This method can safely be called inside an actor message handler,
    constructor, or destructor.
    */
    inline Framework &GetFramework() const;

    /**
    \brief Gets the number of messages queued at this actor, awaiting processing.

    Returns the number of messages currently in the message queue of the actor.
    The messages in the queue are those that have been received by the actor but
    for which registered message handlers have not yet been executed, and so are
    still awaiting processing.
    
    \note If called from a message handler function, the returned count doesn't
    include the message whose receipt triggered the execution of the handler.
    */
    inline uint32_t GetNumQueuedMessages() const;

    /**
    \brief Registers a handler for a specific message type.

    Derived actor classes should call this method to register message handlers.
    Once registered, a message handler is automatically executed whenever the
    owning actor receives a message of the type that the message handler function
    accepts.

    For example this simple HelloWorld actor has a single handler which is executed
    in response to \em hello messages. It's important to note that message handlers
    must currently always take their message parameters by \em reference:

    \code
    class HelloWorld : public Theron::Actor
    {
    public:

        struct HelloMessage { };

        HelloWorld()
        {
            RegisterHandler(this, &HelloWorld::Hello);
        }

    private:

        inline void Hello(const HelloMessage &message, const Theron::Address from)
        {
            printf("Hello world!\n");
        }
    };
    \endcode

    Message handlers can be registered at basically any point in the lifetime of the actor.
    In particular they can safely be registered in the derived actor constructor,
    as well as from within other message handlers of the actor (themselves previously
    registered and then executed in response to received messages).

    If a message handler is registered multiple times, it will be executed multiple times
    in response to received messages of its expected type.

    One subtlety to be aware of is that message handlers aren't executed on receipt of
    messages of types \em derived from the message types expected by the handler. In order to
    handle derived message types, register separate handlers for those types - and call the handler
    registered for the base message type explicitly from the derived handler, if required.

    \code
    class HelloWorld : public Theron::Actor
    {
    public:

        struct HelloMessage { };
        struct HowdyMessage : public HelloMessage { };

        HelloWorld()
        {
            RegisterHandler(this, &HelloWorld::Hello);
            RegisterHandler(this, &HelloWorld::Howdy);
        }

    private:

        inline void Hello(const HelloMessage &message, const Theron::Address from)
        {
            printf("Hello world!\n");
        }

        inline void Howdy(const HowdyMessage &message, const Theron::Address from)
        {
            Hello(message, from);
            printf("How y'all doin?\n");
        }
    };
    \endcode

    Another tricky subtlety is that \em identical handlers are sometimes treated as the same
    function, by the compiler, in optimized builds. The following example shows this
    confusing effect:

    \code
    class HelloWorld : public Theron::Actor
    {
    public:

        struct HelloMessage { };

        HelloWorld()
        {
            RegisterHandler(this, &HelloWorld::HelloOne);
            IsHandlerRegistered(this, &HelloWorld::HelloTwo);   // Returns true, if some optimized builds.
        }

    private:

        inline void HelloOne(const HelloMessage &message, const Theron::Address from)
        {
            printf("Hello world!\n");
        }

        inline void HelloTwo(const HelloMessage &message, const Theron::Address from)
        {
            printf("Hello world!\n");
        }
    };
    \endcode

    \tparam ActorType The derived actor class.
    \tparam ValueType The message type accepted by the handler.
    \param actor Pointer to the derived actor instance.
    \param handler Member function pointer identifying the message handler function.
    \return True, if the registration was successful. Failure may indicate out-of-memory.

    \see <a href="http://www.theron-library.com/index.php?t=page&p=HandlingMessages">Handling messages</a>
    \see <a href="http://www.theron-library.com/index.php?t=page&p=DynamicHandlerRegistration">Dynamic handler registration</a>
    */
    template <class ActorType, class ValueType>
    inline bool RegisterHandler(
        ActorType *const actor,
        void (ActorType::*handler)(const ValueType &message, const Address from));

    /**
    \brief Deregisters a previously registered message handler.

    Call this method to deregister message handlers registered earlier with \ref RegisterHandler.

    Message handlers can be deregistered at any point in the actor's lifetime.
    They can be safely deregistered from within the constructor and destructor of the actor.
    They can also be deregistered from within message handlers, including from within the
    deregistered message handler itself: Message handlers can deregister themselves!
    
    The ability to register and deregister message handlers from \em within message handlers means
    we can dynamically alter the message interface of an actor at runtime, in response to the
    messages we receive. In this example, a HelloWorld actor can be woken and put to sleep
    using messages. It only responds to \em hello messages when awake:

    \code
    class HelloWorld : public Theron::Actor
    {
    public:

        struct WakeMessage { };
        struct SleepMessage { };
        struct HelloMessage { };

        HelloWorld()
        {
            RegisterHandler(this, &HelloWorld::Wake);
        }

    private:

        inline void Wake(const WakeMessage &message, const Theron::Address from)
        {
            RegisterHandler(this, &HelloWorld::Hello);

            DeregisterHandler(this, &HelloWorld::Wake);
            RegisterHandler(this, &HelloWorld::Sleep);
        }

        inline void Sleep(const SleepMessage &message, const Theron::Address from)
        {
            DeregisterHandler(this, &HelloWorld::Hello);

            DeregisterHandler(this, &HelloWorld::Sleep);
            RegisterHandler(this, &HelloWorld::Wake);
        }

        inline void Hello(const HelloMessage &message, const Theron::Address from)
        {
            printf("Hello world!\n");
        }
    };
    \endcode

    If a message handler was previously registered multiple times, it must be deregistered multiple
    times in order to completely deregister it. Each deregistration removes one of the multiple
    registrations, decreasing by one the number of times the handler will be executed in response
    to a received message of the type it expects.

    \tparam ActorType The derived actor class.
    \tparam ValueType The message type accepted by the handler function.
    \param actor Pointer to the derived actor instance.
    \param handler Member function pointer identifying the message handler function.
    \return True, if the message handler was deregistered.

    \note It's not neccessary to deregister handlers that an actor registers before the
    actor is destructed. Doing so is optional and of course completely safe, but if any handlers
    are left registered they will simply be automatically deregistered on destruction of the
    actor.

    \see <a href="http://www.theron-library.com/index.php?t=page&p=DynamicHandlerRegistration">Dynamic handler registration</a>
    */
    template <class ActorType, class ValueType>
    inline bool DeregisterHandler(
        ActorType *const actor,
        void (ActorType::*handler)(const ValueType &message, const Address from));

    /**
    \brief Checks whether the given message handler is registered with the actor.

    Although handlers are typically only registered once, in general they can be registered
    any number of times. For that reason, a handler is counted as registered if the number
    of times it has been \ref RegisterHandler "registered" thus far is greater than the number
    of times it has been \ref DeregisterHandler "deregistered".
    */
    template <class ActorType, class ValueType>
    inline bool IsHandlerRegistered(
        ActorType *const actor,
        void (ActorType::*handler)(const ValueType &message, const Address from));

    /**
    \brief Sets the default message handler executed for unhandled messages.

    This handler, if set, is run when a message arrives at the actor of a type for
    which no regular message handlers are registered.

    \code
    class Actor : public Theron::Actor
    {
    public:

        Actor()
        {
            SetDefaultHandler(this, &Actor::DefaultHandler);
        }

    private:

        inline void DefaultHandler(const Theron::Address from)
        {
            printf("Actor received unknown message from address '%d'\n", from.AsInteger());
        }
    };
    \endcode

    Passing 0 to this method clears any previously set default handler. If no default handler
    is set, then unhandled messages are passed to the
    \ref Framework::SetFallbackHandler "fallback handler" registered with the owning \ref Framework.
    The default fallback handler reports unhandled messages by means of asserts, and can be enabled or
    disabled by the \ref THERON_ENABLE_UNHANDLED_MESSAGE_CHECKS define.

    \tparam ActorType The derived actor class.
    \param actor Pointer to the derived actor instance.
    \param handler Member function pointer identifying the message handler function.

    \see <a href="http://www.theron-library.com/index.php?t=page&p=DefaultMessageHandler">Default message handlers</a>
    */
    template <class ActorType>
    inline bool SetDefaultHandler(
        ActorType *const actor,
        void (ActorType::*handler)(const Address from));

    /**
    \brief Sets the default message handler executed for unhandled messages.

    This method sets a 'blind' default handler which, when executed, is passed the
    unknown message as blind data. The blind data consists of the memory block containing
    the message, identified by a void pointer and a size.

    The blind handler, if set, is run when a message arrives of a type for which no
    regular message handlers are registered. The handler, being user-defined, may be able
    to inspect the contents of the message and take appropriate action, such as reporting
    the contents of an unexpected message to help with debugging.

    \code
    class Actor : public Theron::Actor
    {
    public:

        Actor()
        {
            SetDefaultHandler(this, &Actor::DefaultHandler);
        }

    private:

        inline void DefaultHandler(const void *const data, const Theron::uint32_t size, const Theron::Address from)
        {
            printf("Actor received unknown message of size %d from address '%d'\n", size, from.AsInteger());
        }
    };
    \endcode

    Passing 0 to this method clears any previously set default handler.

    \tparam ActorType The derived actor class.
    \param actor Pointer to the derived actor instance.
    \param handler Member function pointer identifying the message handler function.
    */
    template <class ActorType>
    inline bool SetDefaultHandler(
        ActorType *const actor,
        void (ActorType::*handler)(const void *const data, const uint32_t size, const Address from));

    /**
    \brief Sends a message to the entity (actor or receiver) at the given address.

    \code
    class Responder : public Theron::Actor
    {
    public:

        struct Message
        {
        };

        Responder()
        {
            RegisterHandler(this, &Responder::Respond);
        }

    private:

        inline void Respond(const Message &message, const Theron::Address from)
        {
            // Send the message back to the sender.
            Send(message, from);
        }
    };
    \endcode

    Any copyable class or Plain Old Datatype can be sent in a message. Messages
    are copied when they are sent, so that the recipient sees a different piece of
    memory with the same value. The copying is performed with the copy-constructor
    of the message class.

    In general it is unsafe to pass pointers in messages, since doing so re-introduces
    shared memory, where multiple actors can access the same memory at the same time.
    The exception is where steps are taken to ensure that the sender is prevented from
    accessing the referenced memory after the send (for example using an auto-pointer).
    Effectively ownership is transferred to the recipient.

    Note that C arrays can't be sent as messages, since they aren't copyable. For example
    the first call to Send in the following code won't compile, because the literal string
    message is an array. The second, where the array is sent as a pointer instead, works fine.
    Of course, a safer approach is to send a std::string, as shown as the third call:

    \code
    class Actor : public Theron::Actor
    {
    public:
        Actor()
        {
            Send("hello", someAddress);                 // C array; won't compile!

            const char *const message("hello");
            Send(message, someAddress);                 // Works and fast but use with care.

            Send(std::string("hello"), someAddress);    // Safest but involves a copy.
        }
    }
    \endcode

    If no actor or receiver exists with the given address, the
    message will not be delivered, and \ref Send will return false.

    This can happen if the target entity is an actor and has been garbage collected,
    due to becoming unreferenced. To ensure that actors are not prematurely garbage
    collected, simply hold at least one \ref ActorRef referencing the actor.

    If the destination address exists, but the receiving entity has no handler
    registered for messages of the sent type, then the message will be ignored,
    but this method will still return true.

    Such undelivered or unhandled messages will be caught and reported as asserts by the
    default fallback handler, unless it is replaced by a custom user implementation using
    \ref Framework::SetFallbackHandler.

    This method can safely be called within the constructor or destructor of a derived
    actor object, as well as more typically within its message handler functions.

    \tparam ValueType The message type (any copyable class or Plain Old Datatype).
    \param value The message value to be sent.
    \return True, if the message was delivered to the target entity, otherwise false.

    \note An important detail of message handling in Theron is that message handling
    order is guaranteed. That is, if an actor A sends two messages m1 and m2 successively
    to another actor B, then it's guaranteed that m1 and m2 will arrive at B, and be
    handled, in that order. Theron is able to guarantee this because it currently only
    supports actors within the same process, so message transfer is entirely under its
    control. If Theron is ever extended to support actors across multiple processes, or
    multiple hosts, then this guarantee may have to be relaxed; nevertheless the arrival
    order of messages sent between actors in the same process will still be guaranteed.
    Of course the guarantee doesn't apply to messages sent by different actors: If two
    actors A and B send messages m1 and m2 to a third actor C, there is explicitly no
    guarantee on the arrival order of m1 and m2 at C.

    \see TailSend
    \see <a href="http://www.theron-library.com/index.php?t=page&p=SendingMessages">Sending messages</a>
    */
    template <class ValueType>
    inline bool Send(const ValueType &value, const Address &address) const;

    /**
    \brief Sends a message to the entity at the given address, without waking a worker thread.

    This method sends a message to the entity at a given address, and is functionally
    identical to the similar \ref Send method.
    
    It differs from \ref Send in that it is generally more efficient when called as the last
    operation of a message handler. It should specifically not be used from actor constructors
    or destructors; use \ref Send in those situations instead.

    TailSend causes the recipient of the sent message to be executed by a worker thread
    that is already active, rather than waking a sleeping thread to process it.
    This is useful when called as the last operation of a returning message handler,
    where it is known that the thread executing the message handler is about to become
    available.

    \code
    class Processor : public Theron::Actor
    {
    public:

        Processor()
        {
            RegisterHandler(this, &Processor::Process);
        }

    private:

        inline void Process(const int &message, const Theron::Address from)
        {
            // Do some compute-intensive processing using the message value
            // ...

            // Send the result as the last action using TailSend.
            TailSend(result, from);
        }
    };
    \endcode

    Notionally, TailSend causes the sending and receiving actors to be executed synchronously,
    in series, rather than overlapping the execution of the receiving actor with the execution
    of the remainder of the sending message handler (and any subsequent handlers executed for the
    same message) in the sending actor. When called from the end ('tail') of a message handler,
    the potential overlap is marginal at best so it's faster to not bother waking a thread, and so
    avoid the thread synchronization overheads that waking a thread entails.
    
    \tparam ValueType The message type (any copyable class or Plain Old Datatype).
    \return True, if the message was delivered to the target entity, otherwise false.

    \note Calling this method from positions other than at the end ("tail") of a
    message handler is not generally recommended. In particular it should never be
    called from actor constructors or destructors.

    \see Send
    */
    template <class ValueType>
    inline bool TailSend(const ValueType &value, const Address &address) const;

private:

    typedef Detail::IntrusiveList<Detail::IMessageHandler> MessageHandlerList;

    Actor(const Actor &other);
    Actor &operator=(const Actor &other);

    /// Returns a reference to the core of the actor.
    inline Detail::ActorCore &Core();
    
    /// Returns a const-reference to the core of the actor.
    inline const Detail::ActorCore &Core() const;

    /// Increments the reference count of the actor.
    inline void Reference();

    /// Decrements the reference count of the actor.
    inline void Dereference();

    /// Returns the default message handler, if one is set for this actor.
    inline Detail::IDefaultHandler *GetDefaultHandler() const;

    /// Returns a reference to the list of handlers added since the actor was last processed.
    inline MessageHandlerList &GetNewHandlerList();

    /// Gets a pointer to the pulse counter of the framework that owns the actor.
    uint32_t *GetPulseCounterAddress() const;

    Address mAddress;                                   ///< Unique address of this actor.
    Detail::ActorCore *mCore;                           ///< Pointer to the core implementation of the actor.
    uint32_t mReferenceCount;                           ///< Counts how many ActorRef instances reference this actor.
    Detail::IDefaultHandler *mDefaultMessageHandler;    ///< Handler executed for unhandled messages.
    MessageHandlerList mNewHandlerList;                 ///< Holds new message handlers until they're added the real handler list.
};


THERON_FORCEINLINE Actor::Actor() :
  mAddress(),
  mCore(0),
  mReferenceCount(0),
  mDefaultMessageHandler(0),
  mNewHandlerList()
{
    // Look up the registered member data for this instance in the list.
    Detail::ActorCreator::Entry *const entry(Detail::ActorCreator::Get(this));

    THERON_ASSERT(entry);
    THERON_ASSERT(entry->mLocation == this);
    THERON_ASSERT(entry->mActorCore);
    THERON_ASSERT(entry->mAddress != Address::Null());

    mAddress = entry->mAddress;
    mCore = entry->mActorCore;
}


inline Actor::~Actor()
{
    // Free all allocated handler objects in the new handler list.
    while (Detail::IMessageHandler *const handler = mNewHandlerList.Front())
    {
        mNewHandlerList.Remove(handler);
        AllocatorManager::Instance().GetAllocator()->Free(handler);
    }

    // Free the default handler object, if one is set.
    if (mDefaultMessageHandler)
    {
        AllocatorManager::Instance().GetAllocator()->Free(mDefaultMessageHandler);
    }
}


THERON_FORCEINLINE const Address &Actor::GetAddress() const
{
    return mAddress;
}


THERON_FORCEINLINE Framework &Actor::GetFramework() const
{
    return *mCore->GetFramework();
}


THERON_FORCEINLINE uint32_t Actor::GetNumQueuedMessages() const
{
    // We lock the core mutex to protect access to the message count.
    // Messages may be received during the execution of a handler, changing the count.
    Detail::Lock lock(mCore->GetMutex());
    return mCore->GetNumQueuedMessages();
}


template <class ActorType, class ValueType>
inline bool Actor::RegisterHandler(
    ActorType *const /*actor*/,
    void (ActorType::*handler)(const ValueType &message, const Address from))
{
    typedef Detail::MessageHandler<ActorType, ValueType> MessageHandlerType;

    // Allocate memory for a message handler object.
    void *const memory = AllocatorManager::Instance().GetAllocator()->Allocate(sizeof(MessageHandlerType));
    if (memory == 0)
    {
        return false;
    }

    // Construct a handler object to remember the function pointer and message value type.
    MessageHandlerType *const messageHandler = new (memory) MessageHandlerType(handler);
    THERON_ASSERT(messageHandler);

    // Add the handler to a list for adding later when we're sure no handlers are running.
    // We don't need to lock this because only one thread can access it at a time.
    mNewHandlerList.Insert(messageHandler);

    // Tell the core to update its handler list before processing the actor.
    mCore->DirtyHandlers();

    return true;
}


template <class ActorType, class ValueType>
inline bool Actor::DeregisterHandler(
    ActorType *const actor,
    void (ActorType::*handler)(const ValueType &message, const Address from))
{
    // If the message value type has a valid (non-zero) type name defined for it,
    // then we use explicit type names to match messages to handlers.
    // The default value of zero indicates that no type name has been defined,
    // in which case we rely on compiler-generated RTTI to identify message types.
    typedef Detail::MessageHandler<ActorType, ValueType> MessageHandlerType;
    typedef Detail::MessageHandlerCast<ActorType, Detail::MessageTraits<ValueType>::HAS_TYPE_NAME> HandlerCaster;

    // Remove the handler from the main handler list, if it's there.
    if (mCore->DeregisterHandler<ActorType, ValueType>(actor, handler))
    {
        return true;
    }

    // The handler wasn't in the registered list, maybe it's in the new handlers list.
    // That can happen if the handler was only just registered prior to this in the same function.
    // It's a bit weird to register a handler and the immediately deregister it, but legal.
    typename MessageHandlerList::Iterator newHandlers(mNewHandlerList.Begin());
    const typename MessageHandlerList::Iterator newHandlersEnd(mNewHandlerList.End());

    while (newHandlers != newHandlersEnd)
    {
        Detail::IMessageHandler *const messageHandler(*newHandlers);

        // Try to convert this handler, of unknown type, to the target type.
        const MessageHandlerType *const typedHandler = HandlerCaster:: template CastHandler<ValueType>(messageHandler);
        if (typedHandler)
        {
            // Don't count the handler if it's already marked for deregistration.
            if (typedHandler->GetHandlerFunction() == handler && !typedHandler->IsMarked())
            {
                // Mark the handler for deregistration.
                messageHandler->Mark();

                // We don't need to tell the actor to update its handlers since the new handler list is non-empty.
                THERON_ASSERT(mCore->AreHandlersDirty());

                return true;
            }
        }

        ++newHandlers;
    }

    return false;
}


template <class ActorType, class ValueType>
inline bool Actor::IsHandlerRegistered(
    ActorType *const actor,
    void (ActorType::*handler)(const ValueType &message, const Address from))
{
    // If the message value type has a valid (non-zero) type name defined for it,
    // then we use explicit type names to match messages to handlers.
    // The default value of zero indicates that no type name has been defined,
    // in which case we rely on compiler-generated RTTI to identify message types.
    typedef Detail::MessageHandler<ActorType, ValueType> MessageHandlerType;
    typedef Detail::MessageHandlerCast<ActorType, Detail::MessageTraits<ValueType>::HAS_TYPE_NAME> HandlerCaster;

    // Search the currently registered handlers first.
    if (mCore->IsHandlerRegistered<ActorType, ValueType>(actor, handler))
    {
        return true;
    }

    // The handler wasn't in the registered list, maybe it's in the new handlers list.
    typename MessageHandlerList::Iterator newHandlers(mNewHandlerList.Begin());
    const typename MessageHandlerList::Iterator newHandlersEnd(mNewHandlerList.End());

    while (newHandlers != newHandlersEnd)
    {
        Detail::IMessageHandler *const messageHandler(*newHandlers);

        // Try to convert this handler, of unknown type, to the target type.
        const MessageHandlerType *const typedHandler = HandlerCaster:: template CastHandler<ValueType>(messageHandler);
        if (typedHandler)
        {
            // Count as not registered if it's marked for deregistration.
            // But it may be registered more than once, so keep looking.
            if (typedHandler->GetHandlerFunction() == handler && !typedHandler->IsMarked())
            {
                return true;
            }
        }

        ++newHandlers;
    }

    return false;
}


template <class ActorType>
inline bool Actor::SetDefaultHandler(
    ActorType *const /*actor*/,
    void (ActorType::*handler)(const Address from))
{
    typedef Detail::DefaultHandler<ActorType> HandlerType;

    // Destroy any previously set handler.
    // We don't need to lock this because only one thread can access it at a time.
    if (mDefaultMessageHandler)
    {
        AllocatorManager::Instance().GetAllocator()->Free(mDefaultMessageHandler);
        mDefaultMessageHandler = 0;
    }

    if (handler)
    {
        // Allocate memory for the handler object.
        void *const memory = AllocatorManager::Instance().GetAllocator()->Allocate(sizeof(HandlerType));
        if (memory == 0)
        {
            return false;
        }

        // Construct the handler object to remember the function pointer.
        mDefaultMessageHandler = new (memory) HandlerType(handler);
    }

    return true;
}


template <class ActorType>
inline bool Actor::SetDefaultHandler(
    ActorType *const /*actor*/,
    void (ActorType::*handler)(const void *const data, const uint32_t size, const Address from))
{
    typedef Detail::BlindDefaultHandler<ActorType> HandlerType;

    // Destroy any previously set handler.
    // We don't need to lock this because only one thread can access it at a time.
    if (mDefaultMessageHandler)
    {
        AllocatorManager::Instance().GetAllocator()->Free(mDefaultMessageHandler);
        mDefaultMessageHandler = 0;
    }

    if (handler)
    {
        // Allocate memory for the handler object.
        void *const memory = AllocatorManager::Instance().GetAllocator()->Allocate(sizeof(HandlerType));
        if (memory == 0)
        {
            return false;
        }

        // Construct the handler object to remember the function pointer.
        mDefaultMessageHandler = new (memory) HandlerType(handler);
    }

    return true;
}


THERON_FORCEINLINE Detail::IDefaultHandler *Actor::GetDefaultHandler() const
{
    return mDefaultMessageHandler;
}


THERON_FORCEINLINE Actor::MessageHandlerList &Actor::GetNewHandlerList()
{
    return mNewHandlerList;
}


template <class ValueType>
THERON_FORCEINLINE bool Actor::Send(const ValueType &value, const Address &address) const
{
    Framework *const framework(mCore->GetFramework());
    IAllocator *messageAllocator(AllocatorManager::Instance().GetAllocator());
    uint32_t *pulseCounter(GetPulseCounterAddress());

    // Use the message cache owned by the thread currently executing the actor, if any.
    // When an actor sends a message in its constructor the executing thread is not a worker thread.
    Detail::WorkerContext *const workerContext(mCore->GetWorkerContext());
    if (workerContext != 0)
    {
        messageAllocator = &workerContext->mMessageCache;
        pulseCounter = &workerContext->mPulseCount;
    }

    return Detail::MessageSender::Send(
        messageAllocator,
        pulseCounter,
        framework,
        value,
        mAddress,
        address);
}


template <class ValueType>
THERON_FORCEINLINE bool Actor::TailSend(const ValueType &value, const Address &address) const
{
    Framework *const framework(mCore->GetFramework());
    IAllocator *messageAllocator(AllocatorManager::Instance().GetAllocator());

    // Use the message cache owned by the thread currently executing the actor, if any.
    // When an actor sends a message in its constructor the executing thread is not a worker thread.
    Detail::WorkerContext *const workerContext(mCore->GetWorkerContext());
    if (workerContext != 0)
    {
        messageAllocator = &workerContext->mMessageCache;
    }

    return Detail::MessageSender::TailSend(
        messageAllocator,
        framework,
        value,
        mAddress,
        address);
}


THERON_FORCEINLINE Detail::ActorCore &Actor::Core()
{
    return *mCore;
}


THERON_FORCEINLINE const Detail::ActorCore &Actor::Core() const
{
    return *mCore;
}


THERON_FORCEINLINE void Actor::Reference()
{
    Detail::Lock lock(mCore->GetMutex());
    ++mReferenceCount;
}


THERON_FORCEINLINE void Actor::Dereference()
{
    // We lock the reference count partly to ensure threadsafe access
    // to the actual count and partly to make sure the actor isn't destroyed
    // while we're still accessing it, in the case where we decrement the count to zero.
    Detail::Lock lock(mCore->GetMutex());

    THERON_ASSERT(mReferenceCount > 0);
    if (--mReferenceCount == 0)
    {
        // The framework eventually destroys actors that become unreferenced.
        // However we have to tell the framework that the actor is dead.
        // We call this method to wake a single worker thread.
        // On finding that the actor is unreferenced, the worker thread will destroy it.
        mCore->Unreference();
    }
}


} // namespace Theron


#endif // THERON_ACTOR_H

