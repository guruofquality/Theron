// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_DETAIL_CORE_ACTORCORE_H
#define THERON_DETAIL_CORE_ACTORCORE_H


#include <Theron/Detail/BasicTypes.h>
#include <Theron/Detail/Containers/IntrusiveList.h>
#include <Theron/Detail/Containers/IntrusiveQueue.h>
#include <Theron/Detail/Debug/Assert.h>
#include <Theron/Detail/Handlers/MessageHandler.h>
#include <Theron/Detail/Handlers/IMessageHandler.h>
#include <Theron/Detail/Handlers/MessageHandlerCast.h>
#include <Theron/Detail/Messages/IMessage.h>
#include <Theron/Detail/Messages/MessageTraits.h>

#include <Theron/Address.h>
#include <Theron/Defines.h>
#include <Theron/IAllocator.h>


namespace Theron
{


class Actor;
class Framework;


namespace Detail
{


/// Core functionality of an actor, not exposed to actor implementations.
class ActorCore
{
public:

    friend class Actor;
    friend class Framework;

    /// Default constructor.
    /// \note Actor cores can't be constructed directly in user code.
    ActorCore();

    /// Constructor.
    /// \note Actor cores can't be constructed directly in user code.
    ActorCore(const uint32_t sequence, Framework *const framework, void *const owner, Actor *const actor);

    /// Destructor.
    ~ActorCore();

    /// Sets the pointer to the next actor in a queue of actors.
    THERON_FORCEINLINE void SetNext(ActorCore *const next)
    {
        mNext = next;
    }

    /// Gets the pointer to the next actor in a queue of actors.
    THERON_FORCEINLINE ActorCore *GetNext() const
    {
        return mNext;
    }

    /// Sets the sequence number of the actor.
    THERON_FORCEINLINE void SetSequence(const uint32_t sequence)
    {
        mSequence = sequence;
    }

    /// Gets the sequence number of the actor.
    THERON_FORCEINLINE uint32_t GetSequence() const
    {
        return mSequence;
    }

    /// Pushes a message into the actor.
    THERON_FORCEINLINE void Push(IMessage *const message)
    {
        THERON_ASSERT(message);

        // Push the message onto the internal queue to await delivery.
        // The point of this is to maintain correct message delivery order within the actor.
        // The queue stores pointers to the IMessage interfaces for polymorphism.
        // We reuse the work queue lock to protect the per-actor message queues.
        mMessageQueue.Push(message);
        ++mMessageCount;
    }

    /// Returns a pointer to the derived actor object that owns this core.
    THERON_FORCEINLINE void *GetOwner() const
    {
        return mOwner;
    }

    /// Returns a pointer to the Actor baseclass component of the derived actor object that owns this core.
    THERON_FORCEINLINE Actor *GetParent() const
    {
        return mParent;
    }

    /// Gets the framework to which this actor belongs. Provides derived actor class
    /// implementations with access the owning framework.
    /// \return A pointer to the framework to which the actor belongs.
    THERON_FORCEINLINE Framework *GetFramework() const
    {
        THERON_ASSERT(mFramework);
        return mFramework;
    }

    /// Gets a reference to the mutex that protects the actor.
    Mutex &GetMutex() const;

    /// Returns true if the actor is marked as referenced by at least one ActorRef.
    THERON_FORCEINLINE bool IsReferenced() const
    {
        return ((mState & STATE_REFERENCED) != 0);
    }

    /// Marks the actor as unreferenced and so ready for garbage collection.
    void Unreference();

    /// Returns true if the actor is being processed, or is scheduled for processing.
    THERON_FORCEINLINE bool IsScheduled() const
    {
        return ((mState & STATE_BUSY) != 0);
    }

    /// Marks the actor as either being processed, or scheduled for processing.
    THERON_FORCEINLINE void Schedule()
    {
        mState |= STATE_BUSY;
    }

    /// Marks the actor as neither being processed nor scheduled for processing.
    THERON_FORCEINLINE void Unschedule()
    {
        mState &= STATE_BUSY;
    }

    /// Returns true if the actor is marked as in need of processing.
    THERON_FORCEINLINE bool IsDirty() const
    {
        return ((mState & STATE_DIRTY) != 0);
    }

    /// Marks the actor as in need of processing, causing it to be scheduled.
    THERON_FORCEINLINE void Dirty()
    {
        mState |= STATE_DIRTY;
    }

    /// Marks the actor as not in need of processing.
    THERON_FORCEINLINE void Clean()
    {
        mState &= ~STATE_DIRTY;
    }

    /// Marks the actor as being processed but not yet in need of further processing.
    THERON_FORCEINLINE void CleanAndSchedule()
    {
        uint32_t state(mState);
        state &= ~STATE_DIRTY;
        state |= STATE_BUSY;
        mState = state;
    }

    /// Marks the actor as neither being processed nor in need of processing.
    THERON_FORCEINLINE void CleanAndUnschedule()
    {
        mState &= ~(STATE_DIRTY | STATE_BUSY);
    }

    /// Returns true if the actor has been notified that its handlers need updating.
    THERON_FORCEINLINE bool AreHandlersDirty() const
    {
        return ((mState & STATE_HANDLERS_DIRTY) != 0);
    }

    /// Marks that the actor's handler list has changed since it was last processed.
    THERON_FORCEINLINE void DirtyHandlers()
    {
        mState |= STATE_HANDLERS_DIRTY;
    }

    /// Deregisters a previously registered handler.
    template <class ActorType, class ValueType>
    inline bool DeregisterHandler(
        ActorType *const actor,
        void (ActorType::*handler)(const ValueType &message, const Address from));

    /// Checks whether the given message handler is registered.
    template <class ActorType, class ValueType>
    inline bool IsHandlerRegistered(
        ActorType *const actor,
        void (ActorType::*handler)(const ValueType &message, const Address from));

    /// Removes any handlers marked for removal and adds any scheduled for adding.
    inline void ValidateHandlers();

    /// Gets the number of messages queued at this actor, awaiting processing.
    inline uint32_t GetNumQueuedMessages() const;

    /// Checks whether the actor has any queued messages awaiting processing.
    inline bool HasQueuedMessage() const;

    /// Gets the first message from the message queue, if any.
    inline IMessage *GetQueuedMessage();

    /// Presents the actor with one of its queued messages, if any,
    /// and calls the associated handler.
    inline void ProcessMessage(IMessage *const message);

private:

    /// Flags describing the execution state of an actor.
    enum
    { 
        STATE_BUSY = (1 << 0),                  ///< Being processed (in the work queue or being executed).
        STATE_DIRTY = (1 << 1),                 ///< In need of more processing after current execution.
        STATE_HANDLERS_DIRTY = (1 << 2),        ///< One or more message handlers added or removed since last run.
        STATE_REFERENCED = (1 << 3),            ///< Actor is referenced by one or more ActorRefs so can't be garbage collected.
        STATE_FORCESIZEINT = 0xFFFFFFFF         ///< Ensures the enum is an integer.
    };

    typedef IntrusiveQueue<IMessage> MessageQueue;
    typedef IntrusiveList<IMessageHandler> MessageHandlerList;

    ActorCore(const ActorCore &other);
    ActorCore &operator=(const ActorCore &other);

    /// Updates the core's registered handler list with any changes from the actor.
    void UpdateHandlers();

    /// Executes the core's default handler, if any, for an unhandled message.
    bool ExecuteDefaultHandler(IMessage *const message);

    /// Executes the framework's fallback handler, if any, for an unhandled message.
    bool ExecuteFallbackHandler(IMessage *const message);

    ActorCore *mNext;                           ///< Pointer to the next actor in a queue of actors.
    void *mOwner;                               ///< Pointer to the derived actor object that owns this core.
    Actor *mParent;                             ///< Pointer to the Actor baseclass component of the owning derived actor object.
    Framework *mFramework;                      ///< The framework instance that owns this actor.
    uint32_t mSequence;                         ///< Sequence number of the actor (half of its unique address).
    uint32_t mMessageCount;						///< Number of messages in the message queue.
    MessageQueue mMessageQueue;                 ///< Queue of messages awaiting processing.
    MessageHandlerList mMessageHandlers;        ///< List of registered message handlers.
    uint32_t mState;                            ///< Execution state (idle, busy, dirty).
};


template <class ActorType, class ValueType>
inline bool ActorCore::DeregisterHandler(
    ActorType *const /*actor*/,
    void (ActorType::*handler)(const ValueType &message, const Address from))
{
    // If the message value type has a valid (non-zero) type name defined for it,
    // then we use explicit type names to match messages to handlers.
    // The default value of zero indicates that no type name has been defined,
    // in which case we rely on compiler-generated RTTI to identify message types.
    typedef MessageHandler<ActorType, ValueType> MessageHandlerType;
    typedef MessageHandlerCast<ActorType, MessageTraits<ValueType>::HAS_TYPE_NAME> HandlerCaster;

    // We don't need to lock this because only one thread can access it at a time.
    // Find the handler in the registered handler list.
    typename MessageHandlerList::Iterator handlers(mMessageHandlers.Begin());
    const typename MessageHandlerList::Iterator handlersEnd(mMessageHandlers.End());

    while (handlers != handlersEnd)
    {
        IMessageHandler *const messageHandler(*handlers);

        // Try to convert this handler, of unknown type, to the target type.
        const MessageHandlerType *const typedHandler = HandlerCaster:: template CastHandler<ValueType>(messageHandler);
        if (typedHandler)
        {
            // Don't count the handler if it's already marked for deregistration.
            if (typedHandler->GetHandlerFunction() == handler && !typedHandler->IsMarked())
            {
                // Mark the handler for deregistration.
                // We defer the actual deregistration and deletion until
                // after all active message handlers have been executed, because
                // message handlers themselves can deregister handlers.
                messageHandler->Mark();

                // Mark the handlers as dirty so we update them before the next processing.
                DirtyHandlers();

                return true;
            }
        }
        
        ++handlers;
    }

    return false;
}


template <class ActorType, class ValueType>
inline bool ActorCore::IsHandlerRegistered(
    ActorType *const /*actor*/,
    void (ActorType::*handler)(const ValueType &message, const Address from))
{
    typedef MessageHandler<ActorType, ValueType> MessageHandlerType;
    typedef MessageHandlerCast<ActorType, MessageTraits<ValueType>::HAS_TYPE_NAME> HandlerCaster;

    // Search for the handler in the registered handler list.
    typename MessageHandlerList::Iterator handlers(mMessageHandlers.Begin());
    const typename MessageHandlerList::Iterator handlersEnd(mMessageHandlers.End());

    while (handlers != handlersEnd)
    {
        IMessageHandler *const messageHandler(*handlers);

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

        ++handlers;
    }

    return false;
}


THERON_FORCEINLINE uint32_t ActorCore::GetNumQueuedMessages() const
{
    return mMessageCount;
}


THERON_FORCEINLINE bool ActorCore::HasQueuedMessage() const
{
    return (mMessageCount > 0);
}


THERON_FORCEINLINE IMessage *ActorCore::GetQueuedMessage()
{
    // This exploits the fact that bools are either 0 or 1 to avoid a branch.
    IMessage *const message(mMessageQueue.Pop());
    const bool gotMessage(message != 0);
    const uint32_t messageDecrement(static_cast<uint32_t>(gotMessage));

    THERON_ASSERT(messageDecrement <= 1);
    THERON_ASSERT(mMessageCount >= messageDecrement);

    mMessageCount -= messageDecrement;
    return message;
}


THERON_FORCEINLINE void ActorCore::ValidateHandlers()
{
    if ((mState & STATE_HANDLERS_DIRTY) != 0)
    {
        mState &= (~STATE_HANDLERS_DIRTY);
        UpdateHandlers();
    }
}


THERON_FORCEINLINE void ActorCore::ProcessMessage(IMessage *const message)
{
    THERON_ASSERT(message);

    // Give each registered handler a chance to handle this message.
    bool handled(false);

    MessageHandlerList::Iterator handlersIt(mMessageHandlers.Begin());
    const MessageHandlerList::Iterator handlersEnd(mMessageHandlers.End());

    while (handlersIt != handlersEnd)
    {
        const IMessageHandler *const handler(*handlersIt);
        handled |= handler->Handle(mParent, message);

        ++handlersIt;
    }

    if (handled)
    {
        return;
    }

    // If no registered handler handled the message, execute the default handler instead.
    if (ExecuteDefaultHandler(message))
    {
        return;
    }

    // Finally if the actor has no default handler then run the framework's fallback handler.
    ExecuteFallbackHandler(message);
}


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_CORE_ACTORCORE_H

