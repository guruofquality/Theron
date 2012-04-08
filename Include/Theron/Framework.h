// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_FRAMEWORK_H
#define THERON_FRAMEWORK_H


/**
\file Framework.h
Framework that hosts and executes actors.
*/


#include <Theron/Detail/BasicTypes.h>
#include <Theron/Detail/Core/ActorConstructor.h>
#include <Theron/Detail/Core/ActorCore.h>
#include <Theron/Detail/Core/ActorCreator.h>
#include <Theron/Detail/Debug/Assert.h>
#include <Theron/Detail/Handlers/BlindFallbackHandler.h>
#include <Theron/Detail/Handlers/DefaultFallbackHandler.h>
#include <Theron/Detail/Handlers/FallbackHandler.h>
#include <Theron/Detail/Handlers/IFallbackHandler.h>
#include <Theron/Detail/Messages/IMessage.h>
#include <Theron/Detail/Messages/MessageSender.h>
#include <Theron/Detail/Threading/Mutex.h>
#include <Theron/Detail/ThreadPool/ThreadPool.h>

#include <Theron/ActorRef.h>
#include <Theron/Address.h>
#include <Theron/AllocatorManager.h>
#include <Theron/Defines.h>
#include <Theron/Receiver.h>


namespace Theron
{


/**
\brief Manager class that hosts, manages, and executes actors.

Users should construct an instance of the Framework in non-actor application
code before creating any actors. \ref Actor "Actors" can then be created by calling
the \ref CreateActor method of the instantiated framework. For example:

\code
class MyActor : public Theron::Actor
{
};

Theron::Framework framework;
Theron::ActorRef myActor(framework.CreateActor<MyActor>());
\endcode

Internally, each framework contains a pool of worker threads which are used
to execute the message handlers of the actors created within it. The threads
within a framework are dedicated to executing actors within that framework,
and are never used to execute actors in other frameworks or any other user code.

The initial number of worker threads can be specified on construction of the
framework by means of an explicit parameter to the Framework::Framework constructor.
Additionally, the number of threads can be increased or decreased at runtime
by calling \ref SetMinThreads or \ref SetMaxThreads. The utilization of the currently
enabled threads is measured by performance metrics, queried by \ref GetCounterValue
and enumerated by \ref Counter. The maximum number of threads allowed in any single
framework is limited by \ref THERON_MAX_THREADS_PER_FRAMEWORK.

The worker threads are created and synchronized using underlying threading
objects. Different implementations of these threading objects are possible,
allowing Theron to be used in environments with different threading primitives.
Currently, implementations based on Win32 threads, Boost threads and std::thread
are provided. Users can use the \ref THERON_USE_BOOST_THREADS and \ref THERON_USE_STD_THREADS
defines to enable or disable the use of Boost threads and std::thread, respectively.

It's possible to create more than one Framework in an application. Actors created
within each Framework are executed only by the worker threads in the threadpool
of that framework, allowing the threads of a Framework to be effectively dedicated
to particular actors. See the \ref Framework::Framework "constructor" for more
information.

The maximum number of actors that can be created in an application is limited
by the \ref THERON_MAX_ACTORS define, which defines the number of unique
actor addresses. This limit is per-application, rather than per-framework.

\note An important point about Framework objects is that they must always
outlive the actors created within them. See the destructor documentation
for more information.

\see <a href="http://www.theron-library.com/index.php?t=page&p=CreatingAnActor">Creating an Actor</a>
\see <a href="http://www.theron-library.com/index.php?t=page&p=InitializingTheFramework">Initializing the Framework</a>
\see <a href="http://www.theron-library.com/index.php?t=page&p=TerminatingTheFramework">Terminating the Framework</a>
\see <a href="http://www.theron-library.com/index.php?t=page&p=MultipleFrameworks">Using multiple frameworks</a>
*/
class Framework
{
public:

    friend class Detail::ActorCore;
    friend class Detail::MessageSender;

    /**
    \brief Enumerated type that lists event counters available for querying.

    The counters measure threadpool activity levels, so are useful for
    managing the size of the Framework's internal threadpool.
    
    The \ref COUNTER_THREADS_PULSED counter counts the number of times the threadpool was pulsed
    to wake a worker thread. The pool is pulsed when a message arrives at an actor that is neither
    already being processed nor already scheduled for processing (for another message). Additionally,
    the pool is pulsed when an actor becomes unreferenced and so needs to be garbage collected.
    An important subtlety of \ref COUNTER_THREADS_PULSED is that not all message arrivals are
    counted: only those where the arrived message could be processed immediately if a sleeping
    thread could be woken. Messages that arrive at actors which are already scheduled for processing
    for earlier messages can't be processed immediately anyway, since messages are processed one at a
    time (serially) within each actor.

    The effect of pulsing the pool is to wake a single worker thread, if one or more sleeping threads
    are available. If all worker threads are already awake, then the pulse has no effect. The
    \ref COUNTER_THREADS_WOKEN counter counts the number of times a sleeping thread was actually
    woken by a pulse event. Therefore, the value of the \ref COUNTER_THREADS_WOKEN is strictly
    less than, or equal to, the value of the \ref COUNTER_THREADS_PULSED counter.

    By comparing the values of \ref COUNTER_THREADS_WOKEN and \ref COUNTER_THREADS_PULSED, we
    can get a feeling for whether adding more threads would speed up message handling throughput
    and reduce latency. Conversely, it tells us whether reducing the size of the threadpool would
    have a negative effect on message processing.
    
    If the values of \ref COUNTER_THREADS_WOKEN and \ref COUNTER_THREADS_PULSED are equal, we deduce
    that the current number of worker threads is sufficient to allow every message to be processed
    as quickly as message arrival order allows. To the degree that \ref COUNTER_THREADS_WOKEN is
    less than \ref COUNTER_THREADS_PULSED, we know that adding more worker threads would allow
    more messages to be processed in parallel in software, reducing message response times. (Absolute
    throughput is of course still limited by the number of physical cores).

    Finally, the \ref COUNTER_MESSAGES_PROCESSED counter counts the number of messages that
    were processed by all threads in the threadpool. This gives a rough indication of workload.

    \note All the counters are local to each Framework instance, and count events in
    the queried Framework only.

    \see GetCounterValue
    */
    enum Counter
    {
        COUNTER_MESSAGES_PROCESSED = 0,     ///< Number of arrived actor messages processed by the framework.
        COUNTER_THREADS_PULSED,             ///< Number of times the framework pulsed its threadpool to wake a thread.
        COUNTER_THREADS_WOKEN,              ///< Number of threads actually woken by pulse events.
        MAX_COUNTERS                        ///< Number of counters available for querying.
    };

    /**
    \brief Default constructor.

    Constructs a framework object, with two worker threads by default.

    \note To create a Framework with a specific number of worker threads, use the
    explicit constructor that takes a thread count as an argument.
    */
    Framework();

    /**
    \brief Constructor.

    Constructs a framework with the given number of worker threads.

    It's possible to construct multiple Framework objects within a single
    application. The actors created within each Framework are processed
    by separate pools of worker threads. Actor addresses are globally unique
    across all frameworks. Actors in one framework can send messages to actors
    in other frameworks.

    \code
    class MyActor : public Theron::Actor
    {
    };

    Theron::Framework frameworkOne;
    Theron::ActorRef actorOne(frameworkOne.CreateActor<MyActor>());

    Theron::Framework frameworkTwo;
    Theron::ActorRef actorTwo(frameworkTwo.CreateActor<MyActor>());
    \endcode
    */
    explicit Framework(const uint32_t numThreads);
    
    /**
    \brief Destructor.

    Destroys a Framework.

    A Framework must always outlive (ie. be destructed after) any actors created within
    it. Specifically, all \ref ActorRef objects referencing actors created within a Framework
    must have been destructed before the Framework itself is allowed to be destructed.
    The \ref ActorRef objects returned by \ref CreateActor reference the actors created within
    the Framework. When the last ActorRef referencing an actor is destructed, the actor is
    destroyed automatically by a garbage collection thread within the Framework. For this to
    work correctly, the Framework must still when the actor becomes unreferenced. If the
    Framework is allowed to be destructed before the ActorRefs referencing its created actors,
    the referenced actors will not be correctly destroyed, leading to memory leaks.

    See the documentation of \ref ActorRef for more details.
    */
    ~Framework();

    /**
    \brief Creates an \ref Actor within the framework.

    \code
    class MyActor : public Theron::Actor
    {
    };

    Theron::Framework framework;
    Theron::ActorRef myActor = framework.CreateActor<MyActor>();
    \endcode

    The maximum number of actors that can be created in an application (across all
    frameworks) is limited by the \ref THERON_MAX_ACTORS define, whose default value
    is defined in \ref Defines.h.

    \note It is important that the framework in which an actor is created
    must always outlive it. It is the caller's responsibility to not allow
    the owning framework to be destructed (by being explicitly destructed
    or by going out of scope) until all \ref ActorRef "actor references"
    referencing actors created within the framework have been destructed.

    \tparam ActorType The actor class to be instantiated.
    \return An ActorRef referencing the new actor, returned by value.

    \see <a href="http://www.theron-library.com/index.php?t=page&p=CreatingAnActor">Creating an Actor</a>
    */
    template <class ActorType>
    ActorRef CreateActor();

    /**
    \brief Creates an actor with provided parameters.

    Accepts an instance of the Parameters type exposed by the ActorType class,
    which is in turn passed to the ActorType constructor. This provides a way for
    user-defined actor classes to be provided with user-defined parameters on construction.

    \code
    class MyActor : public Theron::Actor
    {
    public:

        typedef int Parameters;

        MyActor(const Parameters &params) : mMember(params)
        {
        }

    private:

        int mMember;
    };

    Theron::Framework framework;
    MyActor::Parameters params(5);
    Theron::ActorRef myActor = framework.CreateActor<MyActor>(params);
    \endcode

    The maximum number of actors that can be created in an application (across all
    frameworks) is limited by the \ref THERON_MAX_ACTORS define, whose default value
    is defined in \ref Defines.h.

    \tparam ActorType The actor class to be instantiated.
    \param params An instance of the Parameters type exposed by the ActorType class.
    \return An ActorRef referencing the new actor, returned by value.

    \note This method can only be used with derived actor classes that expose a
    Parameters type and a constructor that takes an instance of the Parameters type as
    a constructor parameter. The Parameters type is optional and derived actors are not
    required to define it. However defining a Parameters type and a suitable constructor
    allows the objects of the actor type to be initialized open creation, using this
    method.

    \see <a href="http://www.theron-library.com/index.php?t=page&p=InitializingAnActor">Initializing an Actor</a>
    */
    template <class ActorType>
    ActorRef CreateActor(const typename ActorType::Parameters &params);

    /**
    \brief Sends a message to the entity (typically an actor) at the given address.

    \code
    class Actor : public Theron::Actor
    {
    };

    Theron::Framework framework;
    Theron::Receiver receiver;
    Theron::ActorRef actor(framework.CreateActor<Actor>());

    framework.Send(std::string("Hello"), receiver.GetAddress(), actor.GetAddress());
    \endcode

    If no actor or receiver exists with the given address, the
    message will not be delivered, and \ref Send will return false.

    This can happen if the target entity is an actor and has been garbage collected,
    due to becoming unreferenced. To ensure that actors are not prematurely garbage
    collected, hold at least one \ref ActorRef referencing the actor.

    If the destination address exists, but the receiving entity has no handler
    registered for messages of the sent type, then the message will be ignored,
    but this method will still return true.

    \note This method is used mainly to send messages from non-actor code (eg. main),
    where only the Framework instance is available. In such cases, the address of a receiver
    is typically passed as the 'from' address. When sending messages from within an actor,
    it is more natural to use \ref Actor::Send.

    \tparam ValueType The message type.
    \param value The message value.
    \param from The address of the sending entity (typically a receiver).
    \param to The address of the target entity (an actor or a receiver).
    \return True, if the message was delivered to an entity, otherwise false.

    \see <a href="http://www.theron-library.com/index.php?t=page&p=SendingMessages">Sending messages</a>
    */
    template <class ValueType>
    inline bool Send(const ValueType &value, const Address &from, const Address &to) const;

    /**
    \brief Specifies a maximum limit on the number of worker threads enabled in this framework.

    This method allows an application to place an upper bound on the thread count.
    Users can use this method, together with SetMinThreads, to implement a policy for
    framework threadpool management.

    This method will only decrease the actual number of worker threads, never increase it.
    Calling this method is guaranteed to eventually result in the number of threads being
    less than or equal to the specified limit, as long as messages continue to be sent and
    unless a higher minimum limit is specified subsequently.

    If two successive calls specify different maximums, the lower takes effect. If
    conflicting minimum and maximums are specified by subsequent calls to this method
    and SetMinThreads, then the later call wins.

    The idea behind separate minimum and maximum limits, rather than a single method to
    directly set the actual number of threads, is to allow negotiation between multiple
    agents, each with a different interest in the thread count. One may require a certain
    minimum number of threads for its processing, but not care if the actual number of
    threads is higher, while another may wish to impose a maximum limit on the number of
    threads in existence, but be satisfied if there are less.

    \note If the number of threads before the call was higher than the requested maximum,
    there may be an arbitrary delay after calling this method before the number of threads
    drops to the requested value. The number of threads is managed over time, and is not
    guaranteed to be less than or equal to the requested maximum immediately after the call.
    Threads self-terminate on being woken, if they discover that the actual thread count
    is higher than the limit. This means that until some threads are woken by the arrival
    of new messages, the actual thread count will remain unchanged.

    \param count A positive integer - behavior for zero is undefined.

    \see SetMinThreads
    \see <a href="http://www.theron-library.com/index.php?t=page&p=SettingTheThreadCount">Setting the thread count</a>
    */
    inline void SetMaxThreads(const uint32_t count);

    /**
    \brief Specifies a minimum limit on the number of worker threads enabled in this framework.

    This method allows an application to place a lower bound on the thread count.
    Users can use this method, together with SetMaxThreads, to implement a policy for
    framework threadpool management.

    This method will only increase the actual number of worker threads, never reduce it.
    Calling this method is guaranteed to eventually result in the number of threads being
    greater than or equal to the specified limit, unless a lower maximum limit is specified
    subsequently.

    If two successive calls specify different minimums, the higher takes effect. If
    conflicting minimum and maximums are specified by subsequent calls to this method
    and SetMaxThreads, then the later call wins.

    \note If the number of threads before the call was lower than the requested minimum,
    there may be an arbitrary delay after calling this method before the number of threads rises
    to the requested value. Threads are spawned or re-enabled by a manager thread dedicated
    to that task, which runs asynchronously from other threads as a background task.
    It spends most of its time asleep, only being woken by calls to SetMinThreads.

    \param count A positive integer - behavior for zero is undefined.

    \see SetMaxThreads
    \see <a href="http://www.theron-library.com/index.php?t=page&p=SettingTheThreadCount">Setting the thread count</a>
    */
    inline void SetMinThreads(const uint32_t count);

    /**
    \brief Returns the current maximum limit on the number of worker threads in this framework.

    This method returns the current maximum limit on the size of the worker threadpool.
    Setting a maximum thread limit with SetMaxThreads doesn't imply that that limit will be
    returned by this function. The target thread count is negotiated over multiple calls,
    and specifying a higher value than the current maximum may have no effect.

    \note In the current implementation, GetMaxThreads and GetMinThreads return the same
    value, which is the current target thread count. Note that this may be different from
    the actual current number of threads, returned by GetNumThreads.

    \see GetMinThreads
    \see <a href="http://www.theron-library.com/index.php?t=page&p=SettingTheThreadCount">Setting the thread count</a>
    */
    inline uint32_t GetMaxThreads() const;

    /**
    \brief Returns the current minimum limit on the number of worker threads in this framework.

    This method returns the current minimum limit on the size of the worker threadpool.
    Setting a minimum thread limit with SetMinThreads doesn't imply that that limit will be
    returned by this function. The target thread count is negotiated over multiple calls,
    and specifying a lower value than the current minimum may have no effect.

    \see GetMaxThreads
    \see <a href="http://www.theron-library.com/index.php?t=page&p=SettingTheThreadCount">Setting the thread count</a>
    */
    inline uint32_t GetMinThreads() const;

    /**
    \brief Gets the actual number of worker threads currently in this framework.

    The returned count reflects the actual number of enabled threads at the time of the
    call, which is independent from any maximum or minimum limits specified with
    SetMaxThreads or SetMinThreads. The count includes all enabled threads, including any
    that are sleeping due to having no work to do, but not including ones which were
    created earlier but subsequently terminated to reduce the thread count.

    \note The value returned by this method is specific to this framework instance. If
    multiple frameworks are created then each has its own threadpool with an independently
    managed thread count.

    \see GetPeakThreads
    \see <a href="http://www.theron-library.com/index.php?t=page&p=SettingTheThreadCount">Setting the thread count</a>
    */
    inline uint32_t GetNumThreads() const;

    /**
    \brief Gets the peak number of worker threads ever active in the framework.

    This call queries the highest number of simultaneously enabled threads seen since the
    start of the framework. Note that this measures the highest actual number of threads,
    as measured by GetNumThreads, rather than the highest values of the maximum or minimum
    thread count limits.

    \note The value returned by this method is specific to this framework instance. If
    multiple frameworks are created then each has its own threadpool with an independently
    managed thread count.

    \see <a href="http://www.theron-library.com/index.php?t=page&p=SettingTheThreadCount">Setting the thread count</a>
    */
    inline uint32_t GetPeakThreads() const;

    /**
    \brief Resets the \ref Counter "internal event counters" that track reported events for threadpool management.

    \see Counter
    \see GetCounterValue
    \see <a href="http://www.theron-library.com/index.php?t=page&p=MeasuringThreadUtilization">Measuring thread utilization</a>
    */
    inline void ResetCounters() const;

    /**
    \brief Gets the current integer value of a specified event counter.

    Each Framework maintains a set of \ref Counter "internal event counters".
    The event counters measure the utilization level of the threads in the Framework's
    threadpool, and so are useful for implementing threadpool management policies.

    \param counter One of several values of an \ref Counter "enumerated type" identifying the available counters.

    \see ResetCounters
    \see <a href="http://www.theron-library.com/index.php?t=page&p=MeasuringThreadUtilization">Measuring thread utilization</a>
    */
    inline uint32_t GetCounterValue(const Counter counter) const;

    /**
    \brief Sets the fallback message handler executed for unhandled messages.

    The fallback handler registered with the framework is run:
    - when a message is sent to an address at which no entity is registered by an actor within this framework.
    - when a message is sent to an address at which no entity is registered using the \ref Send method of this framework.
    - when a message is delivered to an actor within this framework at which neither a handler for that message type nor a default handler are registered, so that the message is unhandled.

    The main purpose of the fallback handler is for error reporting, and the default
    fallback handler, which is used if no user-defined handler is explicitly set,
    asserts to indicate an unhandled message, which is a common programming error.
    The reporting of unhandled messages by the default fallback handler can be controlled
    by the \ref THERON_ENABLE_UNHANDLED_MESSAGE_CHECKS define, whose default value is
    defined in \ref Defines.h.

    The handler function registered by this method overload is represented by a pointer to
    a user-defined handler object, and a member function pointer identifying a handler
    function that is a member function of the handler object. Handlers registered using
    this method must expose a handler method that accepts a 'from' address, as shown by
    the following example:

    \code
    class Handler
    {
    public:

        inline void Handle(const Theron::Address from)
        {
            printf("Caught undelivered or unhandled message sent from address '%d'\n", from.AsInteger());
        }
    };

    Theron::Framework framework;
    Handler handler;
    framework.SetFallbackHandler(&handler, &Handler::Handle);
    \endcode

    Users can call this method to replace the default fallback handler with their own
    implementation, for example for more sophisticated error reporting or logging.
    Passing 0 to this method clears any previously set fallback handler, or the
    default fallback handler if no user-defined handler has previously been set.

    \tparam ObjectType The type of the handler object which owns the handler function.
    \param actor Pointer to the handler object on which the handler function is a member function.
    \param handler Member function pointer identifying the fallback handler function.
    */
    template <class ObjectType>
    inline bool SetFallbackHandler(
        ObjectType *const actor,
        void (ObjectType::*handler)(const Address from));

    /**
    \brief Sets the fallback message handler executed for unhandled messages.

    This method sets a 'blind' default handler which, when executed, is passed the
    unknown message as blind data. The blind data consists of the memory block containing
    the message, identified by a void pointer and a size.

    The fallback handler registered with the framework is run:
    - when a message is sent to an address at which no entity is registered by an actor within this framework.
    - when a message is sent to an address at which no entity is registered using the \ref Send method of this framework.
    - when a message is delivered to an actor within this framework at which neither a handler for that message type nor a default handler are registered, so that the message is unhandled.

    The handler, being user-defined, may be able to inspect the contents of the message and
    take appropriate action, such as reporting the contents of an unexpected message to help
    with debugging.

    The handler function registered by this method overload is represented by a pointer to
    a user-defined handler object, and a member function pointer identifying a handler
    function that is a member function of the handler object. Handlers registered using
    this method must expose a handler method that accepts a void pointer identifying the
    message contents, a size indicating the size of the message data, and a 'from' address,
    as shown by the following example:

    \code
    class Handler
    {
    public:

        inline void Handle(const void *const data, const Theron::uint32_t size, const Theron::Address from)
        {
            printf("Caught undelivered or unhandled message of size %d sent from address '%d'\n", size, from.AsInteger());
        }
    };

    Theron::Framework framework;
    Handler handler;
    framework.SetFallbackHandler(&handler, &Handler::Handle);
    \endcode

    Users can call this method to replace the default fallback handler with their own
    implementation, for example for more sophisticated error reporting or logging.
    Passing 0 to this method clears any previously set fallback handler, or the
    default fallback handler if no user-defined handler has previously been set.

    \tparam ObjectType The type of the handler object which owns the handler function.
    \param actor Pointer to the handler object on which the handler function is a member function.
    \param handler Member function pointer identifying the fallback handler function.
    */
    template <class ObjectType>
    inline bool SetFallbackHandler(
        ObjectType *const actor,
        void (ObjectType::*handler)(const void *const data, const uint32_t size, const Address from));

private:

    Framework(const Framework &other);
    Framework &operator=(const Framework &other);

    /// Initializes a Framework object on construction.
    inline void Initialize(const uint32_t numThreads);

    /// Gets a reference to the core message processing mutex.
    inline Detail::Mutex &GetMutex() const;

    /// Schedules an actor for processing by the framework's threadpool.
    inline void Schedule(Detail::ActorCore *const actor) const;

    /// Schedules an actor for processing by the framework's threadpool, without waking a worker thread.
    inline void TailSchedule(Detail::ActorCore *const actor) const;

    /// Executes the fallback message handler for a message which was unhandled by an actor.
    inline bool ExecuteFallbackHandler(const Detail::IMessage *const message) const;

    /// Gets the fallback handler registered with this framework, if any.
    inline const Detail::IFallbackHandler *GetFallbackHandler() const;

    mutable Detail::ThreadPool mThreadPool;                 ///< Pool of worker threads used to run actor message handlers.
    Detail::IFallbackHandler *mFallbackMessageHandler;      ///< Registered message handler run for unhandled messages.
    Detail::DefaultFallbackHandler mDefaultFallbackHandler; ///< Default handler for unhandled messages.
};


THERON_FORCEINLINE void Framework::Initialize(const uint32_t numThreads)
{
    // Reference the global free list to ensure it's created.
    Detail::MessageCache::Instance().Reference();

    THERON_ASSERT_MSG(numThreads > 0, "numThreads must be greater than zero");
    mThreadPool.Start(numThreads);

    // Register the default fallback handler initially.
    SetFallbackHandler(&mDefaultFallbackHandler, &Detail::DefaultFallbackHandler::Handle);
}


template <class ActorType>
inline ActorRef Framework::CreateActor()
{
    typedef Detail::ActorConstructor<ActorType, false> ConstructorType;

    const ConstructorType constructor;
    ActorType *const actor = Detail::ActorCreator::CreateActor(constructor, this);

    // If the actor pointer is zero the constructed ActorRef is null.
    return ActorRef(actor);
}


template <class ActorType>
inline ActorRef Framework::CreateActor(const typename ActorType::Parameters &params)
{
    typedef Detail::ActorConstructor<ActorType, true> ConstructorType;

    const ConstructorType constructor(params);
    ActorType *const actor = Detail::ActorCreator::CreateActor(constructor, this);

    // If the actor pointer is zero the constructed ActorRef is null.
    return ActorRef(actor);
}


template <class ValueType>
THERON_FORCEINLINE bool Framework::Send(const ValueType &value, const Address &from, const Address &to) const
{
    return Detail::MessageSender::Send(
        this,
        value,
        from,
        to);
}


template <class ObjectType>
inline bool Framework::SetFallbackHandler(
    ObjectType *const handlerObject,
    void (ObjectType::*handler)(const Address from))
{
    typedef Detail::FallbackHandler<ObjectType> HandlerType;

    // Destroy any previously set handler.
    // We don't need to lock this because only one thread can access it at a time.
    if (mFallbackMessageHandler)
    {
        AllocatorManager::Instance().GetAllocator()->Free(mFallbackMessageHandler);
        mFallbackMessageHandler = 0;
    }

    if (handlerObject && handler)
    {
        // Construct the object to remember the function pointer.
        void *const memory = AllocatorManager::Instance().GetAllocator()->Allocate(sizeof(HandlerType));
        if (memory == 0)
        {
            return false;
        }

        mFallbackMessageHandler = new (memory) HandlerType(handlerObject, handler);
    }

    return true;
}


template <class ObjectType>
inline bool Framework::SetFallbackHandler(
    ObjectType *const handlerObject,
    void (ObjectType::*handler)(const void *const data, const uint32_t size, const Address from))
{
    typedef Detail::BlindFallbackHandler<ObjectType> HandlerType;

    // Destroy any previously set handler.
    // We don't need to lock this because only one thread can access it at a time.
    if (mFallbackMessageHandler)
    {
        AllocatorManager::Instance().GetAllocator()->Free(mFallbackMessageHandler);
        mFallbackMessageHandler = 0;
    }

    if (handlerObject && handler)
    {
        // Construct the object to remember the function pointer.
        void *const memory = AllocatorManager::Instance().GetAllocator()->Allocate(sizeof(HandlerType));
        if (memory == 0)
        {
            return false;
        }

        mFallbackMessageHandler = new (memory) HandlerType(handlerObject, handler);
    }

    return true;
}


THERON_FORCEINLINE void Framework::SetMaxThreads(const uint32_t count)
{
    mThreadPool.SetMaxThreads(count);
}


THERON_FORCEINLINE void Framework::SetMinThreads(const uint32_t count)
{
    mThreadPool.SetMinThreads(count);
}


THERON_FORCEINLINE uint32_t Framework::GetMaxThreads() const
{
    return mThreadPool.GetMaxThreads();
}


THERON_FORCEINLINE uint32_t Framework::GetMinThreads() const
{
    return mThreadPool.GetMinThreads();
}


THERON_FORCEINLINE uint32_t Framework::GetNumThreads() const
{
    return mThreadPool.GetNumThreads();
}


THERON_FORCEINLINE uint32_t Framework::GetPeakThreads() const
{
    return mThreadPool.GetPeakThreads();
}


THERON_FORCEINLINE void Framework::ResetCounters() const
{
    mThreadPool.ResetCounters();
}


THERON_FORCEINLINE uint32_t Framework::GetCounterValue(const Counter counter) const
{
    uint32_t count(0);

    switch (counter)
    {
        case COUNTER_MESSAGES_PROCESSED:
        {
            count = mThreadPool.GetNumMessagesProcessed();
            break;
        }

        case COUNTER_THREADS_PULSED:
        {
            count = mThreadPool.GetNumThreadsPulsed();
            break;
        }

        case COUNTER_THREADS_WOKEN:
        {
            count = mThreadPool.GetNumThreadsWoken();
            break;
        }

        default: break;
    }

    return count;
}


THERON_FORCEINLINE Detail::Mutex &Framework::GetMutex() const
{
    return mThreadPool.GetMutex();
}


THERON_FORCEINLINE void Framework::Schedule(Detail::ActorCore *const actor) const
{
    mThreadPool.Push(actor);
}


THERON_FORCEINLINE void Framework::TailSchedule(Detail::ActorCore *const actor) const
{
    mThreadPool.TailPush(actor);
}


THERON_FORCEINLINE bool Framework::ExecuteFallbackHandler(const Detail::IMessage *const message) const
{
    if (mFallbackMessageHandler)
    {
        mFallbackMessageHandler->Handle(message);
        return true;
    }

    return false;
}


THERON_FORCEINLINE const Detail::IFallbackHandler *Framework::GetFallbackHandler() const
{
    return mFallbackMessageHandler;
}


} // namespace Theron


#endif // THERON_FRAMEWORK_H

