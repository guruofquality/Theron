// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_ACTOR_H
#define THERON_ACTOR_H


/**
\file Actor.h
Actor baseclass.
*/


#include <Theron/Defines.h>

// NOTE: Must include xs.h before standard headers to avoid warnings in MS headers!
#if THERON_XS
#include <xs/xs.h>
#endif // THERON_XS

#include <Theron/Address.h>
#include <Theron/Align.h>
#include <Theron/AllocatorManager.h>
#include <Theron/BasicTypes.h>
#include <Theron/EndPoint.h>
#include <Theron/Framework.h>
#include <Theron/IAllocator.h>

#include <Theron/Detail/Directory/Directory.h>
#include <Theron/Detail/Handlers/DefaultHandlerCollection.h>
#include <Theron/Detail/Handlers/FallbackHandlerCollection.h>
#include <Theron/Detail/Handlers/HandlerCollection.h>
#include <Theron/Detail/Messages/IMessage.h>
#include <Theron/Detail/Messages/MessageCreator.h>
#include <Theron/Detail/Messages/MessageSender.h>
#include <Theron/Detail/Mailboxes/Mailbox.h>
#include <Theron/Detail/MailboxProcessor/Processor.h>
#include <Theron/Detail/Threading/Atomic.h>
#include <Theron/Detail/Threading/Utils.h>


#ifdef _MSC_VER
#pragma warning(push)
#pragma warning (disable:4324)  // structure was padded due to __declspec(align())
#endif //_MSC_VER


/**
Main namespace containing all public API components.
*/
namespace Theron
{


class ActorRef;
class Framework;


namespace Detail
{
class Processor;
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

Although derived actors classes can be constructed directly in user code, they are
always associated with an owning \ref Framework that hosts and executes them.
The owning Framework is provided to the Actor baseclass on construction.

\code
class MyActor : public Theron::Actor
{
public:

    MyActor(Theron::Framework &framework) : Theron::Actor(framework)
    {
    }
};
\endcode

\note A fundamental principle of the Actor Model is that actors should communicate
only by means of messages, both \ref Send "sent by themselves" and \ref RegisterHandler
"handled by their message handlers". Since Actors in Theron are just vanilla C++
classes that derive from the Actor baseclass, it's possible in practice to add
conventional member functions to derived actor classes, breaking the Actor Model
abstraction. However this should be avoided, since it results in ugly code and
shared memory issues, reintroducing the need for traditional thread synchronization.
Resist the temptation to add a method to an actor class and add a message instead.
*/
class Actor
{
public:

    friend class ActorRef;
    friend class Framework;
    friend class Detail::Processor;

    /**
    \brief Default constructor.

    \note Direct default-construction of Actor baseclasses is forbidden. This
    public default constructor is provided for backwards compatibility with the
    legacy actor creation pattern used by versions of Theron prior to version 4.0.
    
    Always use one of the two supported actor creation patterns: the version 4 syntax or
    (if necessary) the deprecated version 3.x syntax, which is still supported for backwards
    compatibility. When writing new code, always use the new, simpler, version 4 syntax.

    In versions of Theron prior to 4.0, actors couldn't be constructed directly
    in user code. Instead you had to ask a Framework to create one for you, using
    the CreateActor method template. Instead of returning the actor itself,
    CreateActor returned a <i>reference</i> to the actor in the form of an \ref ActorRef
    object.

    \code
    // LEGACY CODE!
    class MyActor : public Theron::Actor
    {
    };

    int main()
    {
        Theron::Framework framework;
        Theron::ActorRef actorRef(framework.CreateActor<MyActor>());
    }
    \endcode

    In versions of Theron starting with 4.0, Actors are first-class citizens and
    behave like vanilla C++ objects. They can be constructed directly with no
    call to Framework::CreateActor. Once constructed they are referenced directly
    by user code with no need for ActorRef proxy objects.

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
    }
    \endcode
    */
    Actor();

    /**
    \brief Explicit constructor.

    Constructs an actor within the given \ref Framework, with the given unique name.
    If the name parameter is null then the actor is given an automatically generated name.

    The Actor baseclass constructor expects an instance of the Framework class,
    which becomes the owning Framework for the actor, hosting it and executing its
    message handlers.

    When defining their own derived actor classes, users must arrange to pass a
    Framework object to the baseclass constructor. The typical way to arrange this
    is to just expose the same parameter on the constructor of the derived actor,
    and pass it through to the baseclass constructor in the initializer list:

    The optional name parameter allows constructed actor objects to be given unique,
    user-defined names. These names can then be used to send messages to the actors in
    situations where the sending code has no direct reference to the destination actor, so
    can't query it for its address. This is especially important when sending messages
    to remote actors, where the actor is also required to be constructed in a Framework
    tied to an \ref EndPoint.

    If no name parameter is provided, or the name parameter is null, then the constructed
    actor is given an automatically generated name. The generated name can still be used
    to send messages, but since it is automatically generated it can't be known to the
    user except by calling \ref GetAddress. Note that the generated name is only guaranteed
    to be unique across all processes if each \ref EndPoint in the system is given a unique,
    user-defined name on construction.

    \note Sending a message to an actor using only its name requires an \ref EndPoint.
    It involves a hashed lookup, so is slower than using an address retrieved directly
    from the target actor using \ref GetAddress.

    \code
    class MyActor : public Theron::Actor
    {
    public:

        MyActor(Theron::Framework &framework, const char *const name) :
          Theron::Actor(framework, name)
        {
        }
    };

    Theron::EndPoint::Parameters params("local", "tcp://192.168.10.104:5555");
    Theron::EndPoint endPoint(params);
    Theron::Framework framework(endPoint);
    Theron::Receiver receiver(endPoint, "receiver");

    MyActor actorOne(framework, "actor_one");
    MyActor actorTwo(framework, "actor_two");

    framework.Send(
        std::string("hello"),
        Theron::Address("receiver"),
        Theron::Address("actor_one"));
    \endcode

    A powerful feature of actors is that they can create other actors, allowing
    the building of complex subsystems of actors using object-like abstraction.
    See the documentation of the \ref GetFramework method for more information.

    \param framework Reference to a framework within which the actor will be hosted.
    \param name An optional string defining the unique name of the actor object.

    \note The string name parameter is copied so can be destroyed after the call.
    */
    explicit Actor(Framework &framework, const char *const name = 0);

    /**
    \brief Baseclass virtual destructor.
    */
    virtual ~Actor();

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

        explicit Actor(Theron::Framework &framework) : Theron::Actor(framework)
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
    constructor, or destructor. As a public method, it can also be accessed from
    outside the actor to query the actor's address.
    */
    inline Address GetAddress() const;

    /**
    \brief Returns a reference to the framework that owns the actor.

    This enables an actor to access the framework that owns it. One reason
    this may be useful is to allow the actor to create other actors, which
    effectively become nested inside it as its children. By nesting actors
    it's possible to build complex subsystems, abstracted away behind the
    message interface of a single actor.

    To create actors that are 'children' of another actor, the parent actor
    passes its owning Framework as a parameter to the child actor constructor.

    A typical pattern is the allocate the children dynamically on the heap
    using \em "operator new" in the parent constructor, hold pointers to the
    created children in a container, and then destroy them using \em "operator
    delete" in the parent destructor.

    \code
    class Wheel : public Theron::Actor
    {
        explicit Wheel(Theron::Framework &framework) : Theron::Actor(framework)
        {
        }
    };

    class Car : public Theron::Actor
    {
    public:

        explicit Car(Theron::Framework &framework) : Theron::Actor(framework)
        {
            mWheels[0] = new Wheel(GetFramework());
            mWheels[1] = new Wheel(GetFramework());
            mWheels[2] = new Wheel(GetFramework());
            mWheels[3] = new Wheel(GetFramework());
        }

        Car()
        {
            delete mWheels[0];
            delete mWheels[1];
            delete mWheels[2];
            delete mWheels[3];
        }

    private:

        Wheel *mWheels[4];
    };
    \endcode

    \note This method can safely be called inside an actor message handler,
    constructor, or destructor.
    */
    inline Framework &GetFramework() const;

    /**
    \brief Pushes a message into the actor.

    This method is an alternative to \ref Framework::Send, which is the more
    conventional and general way to send messages to actors. It can be
    called in situations where the caller happens to have a direct reference
    to the target actor (for example by means of a local variable or pointer).
    
    \note In general \ref Send should be preferred over Push. When sending messages
    from inside an actor (for example in a message handler), use Actor::Send.
    When sending messages to an actor from non-actor code, prefer Framework::Send
    over Actor::Push, even in situations where you have a direct reference to the
    receiving actor. Although Push looks like an optimization, it uses the same general
    message sending mechanism as Send, and in fact is potentially slower in some cases
    due to there being no thread-specific caches available.

    As with \ref Framework::Send, it's necessary to provide a \em from address
    when calling this method, because the address of the sender isn't implicit
    from the context. It's legal to pass Theron::Address() or Theron::Address::Null(),
    both of which evaluate to the \ref Theron::Address::Null "null address".

    \tparam ValueType The message type (any copyable class or Plain Old Datatype).
    \param value The message value, which is copied.
    \param from The address of the sender.
    \return True, if the actor accepted the message.

    \note The return value of this method should be understood to mean
    just that the message was delivered to the mailbox associated with the actor.
    This doesn't mean necessarily that the actor took any action in response
    to the message. If the actor has no handlers registered for messages of that
    type, then the message will simply be consumed without any effect. In such cases
    this method will still return true. This surprising behavior is a result
    of the asynchronous nature of message passing: the sender doesn't wait
    for the recipient to process the message. It is the sender's responsibility
    to ensure that messages are handled by the actors to which they are sent.
    Actor implementations can also register a default message handler (see
    \ref Actor::SetDefaultHandler). If no default handler is registered then the
    unhandled message will be caught by the Framework's
    \ref Framework::SetFallbackHandler "fallback handler", which, by default,
    asserts.
    */
    template <class ValueType>
    inline bool Push(const ValueType &value, const Address &from);

    /**
    \brief Gets the number of messages queued at this actor, awaiting processing.

    Returns the number of messages currently in the message queue of the actor.
    The messages in the queue are those that have been received by the actor but
    for which registered message handlers have not yet been executed, and so are
    still awaiting processing.
    
    \note If called from a message handler function, the returned count includes
    the message whose receipt triggered the execution of the handler. This behavior
    changed in Theron 4 -- in previous versions, the count didn't include the
    current message.
    */
    inline uint32_t GetNumQueuedMessages() const;

protected:

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

        explicit HelloWorld(Theron::Framework &framework) : Theron::Actor(framework)
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
    in response to received messages of its expected type. Each subsequent de-registration
    then removes just one of the multiple registrations.

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

        explicit HelloWorld(Theron::Framework &framework) : Theron::Actor(framework)
        {
            RegisterHandler(this, &HelloWorld::Hello);
            RegisterHandler(this, &HelloWorld::Howdy);
        }

    private:

        void Hello(const HelloMessage &message, const Theron::Address from)
        {
            printf("Hello world!\n");
        }

        void Howdy(const HowdyMessage &message, const Theron::Address from)
        {
            Hello(message, from);
            printf("How y'all doin?\n");
        }
    };
    \endcode

    Another tricky subtlety is that the compiler may treat handlers that are exactly
    identical as the same function, in optimized builds. The following example shows
    this confusing effect:

    \code
    class HelloWorld : public Theron::Actor
    {
    public:

        struct HelloMessage { };

        explicit HelloWorld(Theron::Framework &framework) : Theron::Actor(framework)
        {
            RegisterHandler(this, &HelloWorld::HelloOne);
            IsHandlerRegistered(this, &HelloWorld::HelloTwo);   // May returns true, in some optimized builds.
        }

    private:

        void HelloOne(const HelloMessage &message, const Theron::Address from)
        {
            printf("Hello world!\n");
        }

        void HelloTwo(const HelloMessage &message, const Theron::Address from)
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
    using messages. It only responds to \em HelloMessage messages when awake:

    \code
    class HelloWorld : public Theron::Actor
    {
    public:

        struct WakeMessage { };
        struct SleepMessage { };
        struct HelloMessage { };

        explicit HelloWorld(Theron::Framework &framework) : Theron::Actor(framework)
        {
            RegisterHandler(this, &HelloWorld::Wake);
        }

    private:

        void Wake(const WakeMessage &message, const Theron::Address from)
        {
            RegisterHandler(this, &HelloWorld::Hello);

            DeregisterHandler(this, &HelloWorld::Wake);
            RegisterHandler(this, &HelloWorld::Sleep);
        }

        void Sleep(const SleepMessage &message, const Theron::Address from)
        {
            DeregisterHandler(this, &HelloWorld::Hello);

            DeregisterHandler(this, &HelloWorld::Sleep);
            RegisterHandler(this, &HelloWorld::Wake);
        }

        void Hello(const HelloMessage &message, const Theron::Address from)
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

    \note It's not necessary to deregister handlers that an actor registers before the
    actor is destructed. Doing so is optional, though of course completely safe. Any handlers
    left registered prior to destruction of the actor will simply be automatically deregistered
    on destruction.
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

        explicit Actor(Theron::Framework &framework) : Theron::Actor(framework)
        {
            SetDefaultHandler(this, &Actor::DefaultHandler);
        }

    private:

        void DefaultHandler(const Theron::Address from)
        {
            printf("Actor received unknown message from address '%d'\n", from.AsInteger());
        }
    };
    \endcode

    Passing 0 to this method clears any previously set default handler.
    
    If no default handler is set, then unhandled messages are passed to the
    \ref Framework::SetFallbackHandler "fallback handler" registered with the owning
    \ref Framework. The default fallback handler reports unhandled messages by means
    of asserts, which can be enabled or disabled using the
    \ref THERON_ENABLE_UNHANDLED_MESSAGE_CHECKS define.

    \tparam ActorType The derived actor class.
    \param actor Pointer to the derived actor instance.
    \param handler Member function pointer identifying the message handler function.
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

        explicit Actor(Theron::Framework &framework) : Theron::Actor(framework)
        {
            SetDefaultHandler(this, &Actor::DefaultHandler);
        }

    private:

        void DefaultHandler(const void *const data, const Theron::uint32_t size, const Theron::Address from)
        {
            printf("Actor received unknown message of size %d from address '%d'\n", size, from.AsInteger());
        }
    };
    \endcode

    Passing 0 to this method clears any previously set default handler.

    If no default handler is set, then unhandled messages are passed to the
    \ref Framework::SetFallbackHandler "fallback handler" registered with the owning
    \ref Framework. The default fallback handler reports unhandled messages by means
    of asserts, which can be enabled or disabled using the
    \ref THERON_ENABLE_UNHANDLED_MESSAGE_CHECKS define.

    \tparam ActorType The derived actor class.
    \param actor Pointer to the derived actor instance.
    \param handler Member function pointer identifying the message handler function.
    */
    template <class ActorType>
    inline bool SetDefaultHandler(
        ActorType *const actor,
        void (ActorType::*handler)(const void *const data, const uint32_t size, const Address from));

    /**
    \brief Sends a message to the entity (actor or Receiver) at the given address.

    \code
    class Responder : public Theron::Actor
    {
    public:

        struct Message
        {
        };

        explicit Responder(Theron::Framework &framework) : Theron::Actor(framework)
        {
            RegisterHandler(this, &Responder::Respond);
        }

    private:

        void Respond(const Message &message, const Theron::Address from)
        {
            // Send the message back to the sender.
            Send(message, from);
        }
    };
    \endcode

    Any copyable class or Plain-Old-Data type can be sent in a message. Messages
    are copied when they are sent, so that the recipient sees a different piece of
    memory with the same value. The copying is performed with the copy-constructor
    of the message class.
    
    \note The requirements for messages sent to remote actors over a network are
    different, and significantly stricter. Currently, message types sent to remote
    actors must be bitwise-copyable via memcpy, so can't contain pointers.

    Typically non-POD message classes should be provided with
    a meaningful default constructor, copy constructor, assignment operator and
    destructor. If the message type is derived from a base type (in an inheritance
    hierarchy of message types) then the base class should typically have a virtual
    destructor.

    In general it is unsafe to pass pointers in messages, since doing so re-introduces
    shared memory, where multiple actors can access the same memory at the same time.
    The exception is where steps are taken to ensure that the sender is prevented from
    accessing the referenced memory after the send (for example using an auto-pointer).
    In such cases ownership is effectively transferred to the recipient by the message.

    Note that immediate C arrays can't be sent as messages, since they aren't copyable.
    For example the first call to Send in the following code won't compile, because the
    literal string message is an array. The second, where the array is sent as a pointer
    instead, works fine. Arrays can safely be sent as members of message classes.
    Of course, a safer approach is to send a std::string, as shown as the final call:

    \code
    class Actor : public Theron::Actor
    {
    public:

        struct Message
        {
            char mBuffer[32];
        }

        explicit Actor(Theron::Framework &framework) : Theron::Actor(framework)
        {
            Send("hello", someAddress);                 // Immediate C array; won't compile!

            char *const message("hello");
            Send(message, someAddress);                 // Fastest, but use with care!

            Message message2;
            strcpy(message2.mBuffer, "hello");
            Send(message2, someAddress);                // Safe but involves a shallow copy.

            Send(std::string("hello"), someAddress);    // Safest but involves a deep copy.
        }
    }
    \endcode

    The boolean return value returned by Send indicates whether the message was delivered.
    Note however that messages sent to actors are in reality addressed to mailboxes, not
    actors themselves. So a return value of true indicates only that the message was
    delivered to a mailbox, which may well be unattended.

    A return value of false indicates that an error occurred and the message was not
    sent. In the case of messages addressed to a Receiver, this may indicate that
    no such receiver is registered (it may have existed once and been destructed).
    In the case of messages addressed to an actor, a false return value more likely
    indicates that the message could not be allocated, due to out-of-memory.

    An important subtlety is that a return value of true indicates only that the message
    was delivered to a mailbox, not that it was delivered to an associated actor, or
    moreover that the receiving actor had message handlers registered for it. On the
    contrary a true return value indicates only that the message was delivered to a mailbox.

    The processing of the mailbox is scheduled asynchronously and typically happens
    some time later, on a different thread. If no actor is registered at the mailbox
    when it is eventually processed, then the message will simply be discarded, even
    though it was delivered successfully to the mailbox. And even if an actor is
    registered at the mailbox, it may have no handlers registered for the message
    type, in which case the message may be silently discarded.

    Such undelivered or unhandled messages will be caught and reported as asserts by the
    default fallback handler, unless it is replaced by a custom user implementation using
    \ref Framework::SetFallbackHandler.

    Messages sent to mailboxes within the local process by the same sender are
    guaranteed to be delivered to those mailboxes in the order they are sent. Once
    delivered, they are guaranteed to processed in the order they were received.
    When messages by different senders or to mailboxes in other processes (potentially
    on other hosts) the arrival order is not guaranteed.

    This method can safely be called within the constructor or destructor of a derived
    actor object, as well as (more typically) within its message handler functions.

    \note When sending messages from message handlers, use \ref TailSend instead, in the
    common case where the sending of the message is the last action of the message
    handler. In such cases, using TailSend can result in significantly higher performance.

    \tparam ValueType The message type (any copyable class or Plain-Old-Data type).
    \param value The message value to be sent.
    \param address The address of the destination Receiver or Actor mailbox.
    \return True, if the message was delivered, otherwise false.

    \see TailSend
    */
    template <class ValueType>
    inline bool Send(const ValueType &value, const Address &address) const;

    /**
    \brief Sends a message to the entity at the given address, without waking a worker thread.

    This method sends a message to the entity at a given address, and is functionally
    identical to the similar \ref Send method.
    
    TailSend differs from Send in that it is potentially more efficient when
    called as the last operation of a message handler. TailSend hints to Theron that
    the message handler sending the message is about to terminate, freeing the worker
    thread that is currently executing it. As a result Theron may choose to schedule
    the actor receiving the message on the same thread, rather than dispatching it
    to an arbitrary, and potentially different, worker thread.

    \note TailSend is only useful when called from the 'tail' of a message handler
    (rather than an actor constructor or destructor) and only when the sent message
    is addressed to an actor within the same \ref Framework. Since the actor receiving
    the message is typically processed by the worker thread that is executing the
    current message handler, the two are effectively serialized with no potential
    for parallelism. For that reason TailSend should not be used to send messages
    to actors that are intended to handle the message in parallel with the executing
    handler.

    \code
    class Processor : public Theron::Actor
    {
    public:

        explicit Processor(Theron::Framework &framework) : Theron::Actor(framework)
        {
            RegisterHandler(this, &Processor::Process);
        }

    private:

        void Process(const int &message, const Theron::Address from)
        {
            // Use Send when sending messages from non-tail positions.
            Send(someRequest, someOtherActor);

            // Do some compute-intensive processing using the message value.
            // ...

            // Send the result as the last action using TailSend.
            TailSend(result, from);
        }
    };
    \endcode
   
    \tparam ValueType The message type (any copyable class or Plain Old Datatype).
    \param value The message value to be sent.
    \param address The address of the destination Actor.
    \return True, if the message was delivered to a mailbox, otherwise false.

    \see Send
    */
    template <class ValueType>
    inline bool TailSend(const ValueType &value, const Address &address) const;

private:

    // Actors are non-copyable.
    Actor(const Actor &other);
    Actor &operator=(const Actor &other);

    /**
    Legacy ActorRef reference counting support.
    */
    inline void Reference() const;
    inline bool Dereference() const;

    /**
    Processes the given message, passing it to handlers registered for its type.
    */
    inline void ProcessMessage(
        Detail::FallbackHandlerCollection *const fallbackHandlers,
        Detail::IMessage *const message);

    /**
    Handle an unhandled message.
    */
    void Fallback(
        Detail::FallbackHandlerCollection *const fallbackHandlers,
        const Detail::IMessage *const message);

    Address mAddress;                                   ///< Unique address of this actor.
    Framework *mFramework;                              ///< Pointer to the framework within which the actor runs.
    Detail::HandlerCollection mMessageHandlers;         ///< The message handlers registered by this actor.
    Detail::DefaultHandlerCollection mDefaultHandlers;  ///< Default message handlers registered by this actor.
    Detail::Processor::Context *mProcessorContext;      ///< Remembers the context of the worker thread processing the actor.

    mutable Detail::Atomic::UInt32 mRefCount;           ///< Reference count to support legacy ActorRef API.
    void *mMemory;                                      ///< Pointer to memory block containing final actor type.
};


// TODO: Force-inline
inline Address Actor::GetAddress() const
{
    return mAddress;
}


THERON_FORCEINLINE Framework &Actor::GetFramework() const
{
    return *mFramework;
}


THERON_FORCEINLINE uint32_t Actor::GetNumQueuedMessages() const
{
    // Get a reference to the mailbox at which this actor is registered.
    const Address address(GetAddress());
    Framework &framework(GetFramework());
    const Detail::Mailbox &mailbox(framework.mMailboxes.GetEntry(address.AsInteger()));

    return mailbox.Count();
}


template <class ActorType, class ValueType>
inline bool Actor::RegisterHandler(
    ActorType *const /*actor*/,
    void (ActorType::*handler)(const ValueType &message, const Address from))
{
    // When an actor registers a handler for a message type, we get told about
    // the type. So we take the opportunity to also register the type against
    // its name with the network endpoint (if the framework is tied to one). This
    // enables us to recognize the type when it arrives in a network message as a block
    // blind of data tagged with a type name.
    if (mFramework->mEndPoint)
    {
        mFramework->mEndPoint->RegisterMessageType<ValueType>();
    }

    return mMessageHandlers.Add(handler);
}


template <class ActorType, class ValueType>
inline bool Actor::DeregisterHandler(
    ActorType *const /*actor*/,
    void (ActorType::*handler)(const ValueType &message, const Address from))
{
    return mMessageHandlers.Remove(handler);
}


template <class ActorType, class ValueType>
inline bool Actor::IsHandlerRegistered(
    ActorType *const /*actor*/,
    void (ActorType::*handler)(const ValueType &message, const Address from))
{
    return mMessageHandlers.Contains(handler);
}


template <class ActorType>
inline bool Actor::SetDefaultHandler(
    ActorType *const /*actor*/,
    void (ActorType::*handler)(const Address from))
{
    return mDefaultHandlers.Set(handler);
}


template <class ActorType>
inline bool Actor::SetDefaultHandler(
    ActorType *const /*actor*/,
    void (ActorType::*handler)(const void *const data, const uint32_t size, const Address from))
{
    return mDefaultHandlers.Set(handler);
}


template <class ValueType>
THERON_FORCEINLINE bool Actor::Send(const ValueType &value, const Address &address) const
{
    // Try to use the processor context owned by a worker thread.
    // The current thread will be a worker thread if this method has been called from a message
    // handler. If it was called from an actor constructor or destructor then the current thread
    // may be an application thread, in which case the stored context pointer will be null.
    // If it is null we fall back to the per-framework context, which is shared between threads.
    // The advantage of using a thread-specific context is that it is only accessed by that
    // single thread so doesn't need to be thread-safe and isn't written by other threads
    // so doesn't cause expensive cache coherency updates between cores.
    Detail::Processor::Context *processorContext(mProcessorContext);
    if (mProcessorContext == 0)
    {
        processorContext = &mFramework->mProcessorContext;
    }

    // Allocate a message. It'll be deleted by the worker thread that handles it.
    Detail::IMessage *const message(Detail::MessageCreator::Create(
        &processorContext->mMessageCache,
        value,
        mAddress));

    if (message)
    {
        // Call the message sending implementation using the acquired processor context.
        return Detail::MessageSender::Send(
            mFramework->mEndPoint,
            processorContext,
            mFramework->GetIndex(),
            message,
            address);
    }

    return false;
}


template <class ValueType>
THERON_FORCEINLINE bool Actor::TailSend(const ValueType &value, const Address &address) const
{
    // Try to use the processor context owned by a worker thread.
    // The current thread will be a worker thread if this method has been called from a message
    // handler. If it was called from an actor constructor or destructor then the current thread
    // may be an application thread, in which case the stored context pointer will be null.
    // If it is null we fall back to the per-framework context, which is shared between threads.
    // The advantage of using a thread-specific context is that it is only accessed by that
    // single thread so doesn't need to be thread-safe and isn't written by other threads
    // so doesn't cause expensive cache coherency updates between cores.
    Detail::Processor::Context *processorContext(mProcessorContext);
    if (mProcessorContext == 0)
    {
        processorContext = &mFramework->mProcessorContext;
    }

    // Allocate a message. It'll be deleted by the worker thread that handles it.
    Detail::IMessage *const message(Detail::MessageCreator::Create(
        &processorContext->mMessageCache,
        value,
        mAddress));

    if (message)
    {
        // Call the message sending implementation using the acquired processor context.
        // We schedule the receiving actor, if any, on the local queue of the processing worker thread, if any.
        return Detail::MessageSender::Send(
            mFramework->mEndPoint,
            processorContext,
            mFramework->GetIndex(),
            message,
            address,
            true);
    }

    return false;
}


template <class ValueType>
THERON_FORCEINLINE bool Actor::Push(const ValueType &value, const Address &from)
{
    // This method isn't typically called a message handler of this actor, so
    // we need to use the processor context associated with the owning framework.
    Detail::Processor::Context *const processorContext(&mFramework->mProcessorContext);

    // Allocate a message. It'll be deleted by the worker thread that handles it.
    Detail::IMessage *const message(Detail::MessageCreator::Create(
        &processorContext->mMessageCache,
        value,
        from));

    if (message)
    {
        // Send the message to the actor's own address.
        return Detail::MessageSender::Send(
            mFramework->mEndPoint,
            processorContext,
            mFramework->GetIndex(),
            message,
            GetAddress());
    }

    return false;
}


THERON_FORCEINLINE void Actor::Reference() const
{
    mRefCount.Increment();
}


THERON_FORCEINLINE bool Actor::Dereference() const
{
    uint32_t currentValue(mRefCount.Load());
    uint32_t newValue(currentValue - 1);
    uint32_t backoff(0);

    THERON_ASSERT(currentValue > 0);

    // Repeatedly try to atomically decrement the reference count.
    // We do this by hand so we can atomically find out the new value.
    while (!mRefCount.CompareExchangeAcquire(currentValue, newValue))
    {
        currentValue = mRefCount.Load();
        newValue = currentValue - 1;
        Detail::Utils::Backoff(backoff);
    }

    // Return true if the new reference count is zero.
    return (newValue == 0);
}


THERON_FORCEINLINE void Actor::ProcessMessage(
    Detail::FallbackHandlerCollection *const fallbackHandlers,
    Detail::IMessage *const message)
{
    if (mMessageHandlers.Handle(this, message))
    {
        return;
    }

    // If no registered handler handled the message, execute the default handlers instead.
    // This call is intentionally not inlined to avoid polluting the generated code with the uncommon case.
    Fallback(fallbackHandlers, message);
}


} // namespace Theron


#ifdef _MSC_VER
#pragma warning(pop)
#endif //_MSC_VER


#endif // THERON_ACTOR_H
