// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_RECEIVER_H
#define THERON_RECEIVER_H


/**
\file Receiver.h
Utility that can receive messages from actors.
*/


#include <new>

#include <Theron/Detail/BasicTypes.h>
#include <Theron/Detail/Containers/IntrusiveList.h>
#include <Theron/Detail/Debug/Assert.h>
#include <Theron/Detail/Handlers/ReceiverHandler.h>
#include <Theron/Detail/Handlers/IReceiverHandler.h>
#include <Theron/Detail/Handlers/ReceiverHandlerCast.h>
#include <Theron/Detail/Messages/IMessage.h>
#include <Theron/Detail/Messages/MessageTraits.h>
#include <Theron/Detail/Threading/Lock.h>
#include <Theron/Detail/Threading/Monitor.h>

#include <Theron/Address.h>
#include <Theron/AllocatorManager.h>
#include <Theron/Defines.h>


namespace UnitTests
{
    class ReceiverTestSuite;
}


namespace Theron
{


namespace Detail
{
    class MessageSender;
}


/**
\brief A standalone entity that can accept messages sent by \ref Actor "actors".

A receiver can be instantiated in non-actor user code. Each receiver has
a unique \ref Address "address" not shared by any actor and can receive \ref
Actor::Send "messages sent to it by actors" (usually in response to \ref Framework::Send
"messages sent to those actors" by the non-actor code). The Receiver provides facilities
for synchronizing with the arrival of an expected message, as well as for registering
message handlers to handle and process messages as they arrive.

Every receiver instantiated in user code is automatically assigned its own unique
\ref Address "address", which enables actors to send messages to it, as if it were
an actor itself.

A Receiver accepts messages sent to it by other entities, counting how many it has
received. It provides methods whereby the thread owning the receiver can query the
count of messages received, mark them as consumed, and block to synchronize with the
arrival of an expected message indicating that some resource or result has become
available.

Receivers also provide the ability to \ref Receiver::RegisterHandler
"register message handlers" to handle the messages they receive, similar to the handler
registration mechanism within actors. This allows the owning thread to register
callback-style handler functions. The registered handlers are executed on the arrival
of messages of the types for which they are registered. The arrived message is passed
to the handler function on execution, allowing it to examine, process, or store its value.

The ability to synchronize with the arrival of messages via \ref Wait allows calling
threads to ensure they examine the results of executed handler functions only after
the messages they handle have arrived, and the associated handlers have been executed.

The maximum number of receivers that can be created in an application is limited
by the \ref THERON_MAX_RECEIVERS define, which defines the number of unique
receiver addresses.

\see <a href="http://www.theron-library.com/index.php?t=page&p=Receiver">Using a Receiver</a>
\see <a href="http://www.theron-library.com/index.php?t=page&p=TerminatingTheFramework">Terminating the Framework</a>
*/
class Receiver
{
public:

    friend class Detail::MessageSender;
    friend class UnitTests::ReceiverTestSuite;

    /**
    \brief Default constructor.

    Constructs a receiver with an automatically-assigned unique address.
    */
    Receiver();

    /**
    \brief Destructor.
    */
    ~Receiver();

    /**
    \brief Gets the unique address of the receiver.
    */
    THERON_FORCEINLINE const Address &GetAddress() const
    {
        return mAddress;
    }

    /**
    \brief Registers a message handler for a specific message type.

    The handler is a member function on a handler object of type ClassType.
    The handler takes as input a message of type ValueType and a \em from address
    indicating the sender of the message. The handler will be executed whenever
    messages of type ValueType are received.

    A common pattern is to register a "catch" handler on a simple "catcher" object
    whose purpose is to catch and hold the value of an arrived message:

    \code
    struct Message
    {
        int mValue;
    };

    class Catcher
    {
    public:

        inline void Catch(const Message &message, const Theron::Address from)
        {
            mMessage = message;
        }

        Message mMessage;
    };

    Theron::Receiver receiver;
    Catcher catcher;

    receiver.RegisterHandler(&catcher, &Catcher::Catch);
    \endcode

    Having registered the catcher, the calling thread then examines the caught message
    after synchronizing with its arrival via a blocking call to \ref Wait:

    \code
    receiver.Wait();
    printf("Caught message with value %d\n", catcher.mMessage.mValue);
    \endcode

    \tparam ClassType The type of the registered handler class.
    \tparam ValueType The type of the handled messages.
    \param owner Pointer to the registered handler object.
    \param handler Member function pointer to the registered handler function.
    \return True, if the handler was successfully registered.

    \see <a href="http://www.theron-library.com/index.php?t=page&p=Receiver">Using a Receiver</a>
    */
    template <class ClassType, class ValueType>
    inline bool RegisterHandler(
        ClassType *const owner,
        void (ClassType::*handler)(const ValueType &message, const Address from));

    /**
    \brief Deregisters a previously registered handler.

    \tparam ClassType The type of the registered handler class.
    \tparam ValueType The type of the handled messages.
    \param owner Pointer to the registered handler object.
    \param handler Member function pointer to the registered handler function.
    \return True, if the handler was successfully deregistered.

    \see RegisterHandler
    */
    template <class ClassType, class ValueType>
    inline bool DeregisterHandler(
        ClassType *const owner,
        void (ClassType::*handler)(const ValueType &message, const Address from));

    /**
    \brief Resets to zero the count of messages received but not yet consumed.

    The internal count of the number of messages that have arrived but not yet
    been consumed (by calls to \ref Wait or \ref Consume) is reset to zero,
    effectively forgetting about all messages that have previously arrived.

    \see Count
    */
    inline void Reset();

    /**
    \brief Returns the number of messages received but not yet consumed.

    The \ref Count method allows a calling thread to query the Receiver's count
    of arrived but unconsumed messages. The count is returned without blocking the
    calling thread or waiting for any messages to be recieved.

    Each Receiver maintains an internal count of the number of messages it
    has received which have not yet been accounted for by calls to \ref Wait
    or \ref Consume. Effectively this is the number of arrived messages of
    which the caller is not yet aware or has not yet synchronized.

    By periodically calling \ref Count, a thread can check for arrived messages
    while also continuing with its own work. If the returned count indicates that
    the receiver contains newly arrived, unconsumed messages, then the calling
    thread can call \ref Consume to mark one or more messages as consumed,
    subtracting them from the count.

    \see Reset
    \see Consume
    */
    inline uint32_t Count() const;

    /**
    \brief Waits until one or more messages arrive at the receiver.

    The Wait method allows a calling thread to wait for, and synchronize with, the arrival
    of an expected message. The \ref Wait method takes an optional parameter specifying
    the maximum number of messages expected, which defaults to one. On calling \ref Wait,
    the calling thread blocks if no messages are available that have not already been consumed,
    or accounted for, by previous calls to Wait or \ref Consume.

    The Receiver maintains an internal count of how many messages have been received but not
    yet consumed, and a call to \ref Wait will return immediately if the call can be
    satisfied by an unconsumed message that has already arrived. This means that a caller
    wishing to synchronize with the arrival of an expected message can safely call \ref Wait,
    irrespective of whether some or all of a number of expected messages may have already
    arrived. If one or more messages have arrived the call simply returns immediately without
    blocking, consuming all arrived messages up to the specified maximum. Otherwise
    it blocks until a message arrives, whereupon the thread is woken and returns.
    
    When a received message becomes available, it is \em consumed and will not be used to
    satisfy any later calls to \ref Wait.
    
    If a maximum count was specified as an argument to \ref Wait then all available messages
    are consumed, up to the specified limit. If no limit is specified then just one message is
    consumed. In either case Wait returns the number of messages consumed, allowing the caller
    to update its own accounting of messages expected and received. By supplying a maximum
    limit greater than one, the caller expecting many messages can consume multiple
    messages in one call, without having to call Wait() for each one in turn.

    \code
    // Wait for the arrival of 10 expected messages, blocking until a message arrives
    // and then consuming all arrived messages in one call before waiting again.
    int outstandingCount(10);
    while (outstandingCount)
    {
        outstandingCount -= receiver.Wait(outstandingCount);
    }
    \endcode

    \note This is a blocking call and, if no message is already available, it will cause
    the calling thread to block indefinitely until a message is received.

    \param max Maximum number of arrived messages to be consumed on this call.
    \return The actual number of arrived messages consumed on this call.
    \see <a href="http://www.theron-library.com/index.php?t=page&p=Receiver">Using a Receiver</a>
    \see <a href="http://www.theron-library.com/index.php?t=page&p=TerminatingTheFramework">Terminating the Framework</a>
    */
    inline uint32_t Wait(const uint32_t max = 1);

    /**
    \brief Consumes any unconsumed messages available on the receiver, up to a specified limit.

    The \ref Consume method consumes any messages that have arrived at the receiver but
    not yet been \em consumed by previous calls to \ref Wait or \ref Consume, up to the
    a specified maximum number. The calling thread returns immediately, without waiting
    for any more messages to arrive, and will return without consuming any messages if none
    are available at the time of the call.

    In conjunction with \ref Count, the \ref Consume method provides a way to consume one
    or more expected messages that have arrived, in a single call without waiting. \ref Consume
    returns the actual number of messages consumed, allowing the caller to update their own
    accounting of arrived messages. The effect of consuming the messages is to subtract them
    from the internal count of unconsumed messages, making them unavailable to further calls
    of \ref Wait and \ref Consume. The internal count can be queried by \ref Count.

    \code
    // Wait for the arrival of 10 expected messages while also doing other work.
    // Repeatedly call Consume to consume any arrived messages without blocking.
    int outstandingCount(10);
    while (outstandingCount)
    {
        outstandingCount -= receiver.Consume(outstandingCount);

        // Do some other work before checking again
        // ...

    }
    \endcode

    \return The number of messages actually consumed, which may be zero.
    */
    inline uint32_t Consume(const uint32_t max);

private:

    typedef Detail::IntrusiveList<Detail::IReceiverHandler> MessageHandlerList;

    Receiver(const Receiver &other);
    Receiver &operator=(const Receiver &other);

    /// \brief Pushes a message into the receiver.
    /// \param message Pointer to the message.
    /// \param wake Indicates whether a worker thread should be woken.
    /// \return True, if the receiver accepted the message.
    /// \note This method is "private" and is not intended for use in user code.
    void Push(Detail::IMessage *const message);

    /// \brief Pushes a message into the receiver.
    /// For receivers, this method behaves identically to Push.
    /// \note This method is "private" and is not intended for use in user code.
    inline void TailPush(Detail::IMessage *const message);

    Address mAddress;                           ///< Unique Theron address, or 'name', of the receiver.
    MessageHandlerList mMessageHandlers;        ///< List of registered message handlers.
    mutable Detail::Monitor mMonitor;           ///< Synchronizes access to the message handlers.
    uint32_t mMessagesReceived;                 ///< Indicates that a message was received.
};


template <class ClassType, class ValueType>
inline bool Receiver::RegisterHandler(
    ClassType *const owner,
    void (ClassType::*handler)(const ValueType &message, const Address from))
{
    // If the message value type has a valid (non-zero) type name defined for it,
    // then we use explicit type names to match messages to handlers.
    // The default value of zero will indicates that no type name has been defined,
    // in which case we rely on automatically stored RTTI to identify message types.
    typedef Detail::ReceiverHandler<ClassType, ValueType> MessageHandlerType;

    // Allocate memory for a message handler object.
    void *const memory = AllocatorManager::Instance().GetAllocator()->Allocate(sizeof(MessageHandlerType));
    if (memory == 0)
    {
        return false;
    }

    // Construct a handler object to remember the function pointer and message value type.
    MessageHandlerType *const messageHandler = new (memory) MessageHandlerType(owner, handler);
    if (messageHandler == 0)
    {
        return false;
    }

    {
        Detail::Lock lock(mMonitor.GetMutex());
        mMessageHandlers.Insert(messageHandler);
    }
    
    return true;
}


template <class ClassType, class ValueType>
inline bool Receiver::DeregisterHandler(
    ClassType *const /*owner*/,
    void (ClassType::*handler)(const ValueType &message, const Address from))
{
    // If the message value type has a valid (non-zero) type name defined for it,
    // then we use explicit type IDs to match messages to handlers.
    // The default value of zero will indicate that no type name has been defined,
    // in which case we rely on automatically stored RTTI to identify message types.
    typedef Detail::ReceiverHandler<ClassType, ValueType> MessageHandlerType;
    typedef Detail::ReceiverHandlerCast<ClassType, Detail::MessageTraits<ValueType>::HAS_TYPE_NAME> HandlerCaster;

    {
        Detail::Lock lock(mMonitor.GetMutex());

        // Find the handler in the registered handler list.
        typename MessageHandlerList::Iterator handlers(mMessageHandlers.Begin());
        const typename MessageHandlerList::Iterator handlersEnd(mMessageHandlers.End());

        while (handlers != handlersEnd)
        {
            Detail::IReceiverHandler *const messageHandler(*handlers);
            
            // Try to convert this handler, of unknown type, to the target type.
            const MessageHandlerType *const typedHandler = HandlerCaster:: template CastHandler<ValueType>(messageHandler);
            if (typedHandler)
            {
                if (typedHandler->GetHandlerFunction() == handler)
                {
                    // Remove the handler from the list.
                    mMessageHandlers.Remove(messageHandler);

                    // Free the handler object, which was allocated on registration.
                    AllocatorManager::Instance().GetAllocator()->Free(messageHandler);

                    return true;
                }
            }

            ++handlers;
        }
    }

    return false;
}


THERON_FORCEINLINE void Receiver::Reset()
{
    Detail::Lock lock(mMonitor.GetMutex());
    mMessagesReceived = 0;
}


THERON_FORCEINLINE uint32_t Receiver::Count() const
{
    uint32_t count(0);

    {
        Detail::Lock lock(mMonitor.GetMutex());
        count = mMessagesReceived;
    }

    return count;
}


THERON_FORCEINLINE void Receiver::TailPush(Detail::IMessage *const message)
{
    // Receivers don't have worker threads so TailPush is identical to Push.
    Push(message);
}


THERON_FORCEINLINE uint32_t Receiver::Wait(const uint32_t max)
{
    Detail::Lock lock(mMonitor.GetMutex());

    THERON_ASSERT(max > 0);

    // If messages were received since the last wait (or creation),
    // then we regard those messages as qualifying and early-exit.
    while (mMessagesReceived == 0)
    {
        // Wait to be woken by an arriving message.
        // This blocks until a message arrives!
        mMonitor.Wait(lock);
    }

    uint32_t numConsumed(mMessagesReceived);
    if (mMessagesReceived > max)
    {
        numConsumed = max;
    }

    mMessagesReceived -= numConsumed;
    return numConsumed;
}


THERON_FORCEINLINE uint32_t Receiver::Consume(const uint32_t max)
{
    Detail::Lock lock(mMonitor.GetMutex());

    uint32_t numConsumed(mMessagesReceived);
    if (mMessagesReceived > max)
    {
        numConsumed = max;
    }

    mMessagesReceived -= numConsumed;
    return numConsumed;
}


} // namespace Theron


#endif // THERON_RECEIVER_H

