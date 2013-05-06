// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_DETAIL_SCHEDULER_NONBLOCKINGQUEUE_H
#define THERON_DETAIL_SCHEDULER_NONBLOCKINGQUEUE_H


#include <Theron/Align.h>
#include <Theron/Assert.h>
#include <Theron/BasicTypes.h>
#include <Theron/Counters.h>
#include <Theron/Defines.h>
#include <Theron/YieldStrategy.h>

#include <Theron/Detail/Containers/Queue.h>
#include <Theron/Detail/Mailboxes/Mailbox.h>
#include <Theron/Detail/Scheduler/YieldImplementation.h>
#include <Theron/Detail/Scheduler/YieldPolicy.h>
#include <Theron/Detail/Threading/Atomic.h>
#include <Theron/Detail/Threading/SpinLock.h>


#ifdef _MSC_VER
#pragma warning(push)
#pragma warning (disable:4324)  // structure was padded due to __declspec(align())
#endif //_MSC_VER


namespace Theron
{
namespace Detail
{


/**
\brief Blocking queue implementation based on spinlocks.
*/
class NonBlockingQueue
{
public:

    /**
    The item type which is queued by the queue.
    */
    typedef Mailbox ItemType;

    /**
    Context structure used to access the queue.
    */
    class ContextType
    {
    public:

        friend class NonBlockingQueue;

        inline ContextType() :
          mRunning(false),
          mShared(false),
          mLocalWorkQueue(0)
        {
        }

    private:

        template <class ValueType>
        struct THERON_PREALIGN(THERON_CACHELINE_ALIGNMENT) Aligned
        {
            ValueType mValue;

        } THERON_POSTALIGN(THERON_CACHELINE_ALIGNMENT);

        bool mRunning;                                      ///< Used to signal the thread to terminate.
        bool mShared;                                       ///< Indicates whether this is the 'shared' context.
        Mailbox *mLocalWorkQueue;                           ///< Local thread-specific single-item work queue.
        YieldImplementation mYield;                         ///< Thread yield strategy implementation.
        Aligned<Atomic::UInt32> mCounters[MAX_COUNTERS];    ///< Array of per-context event counters.
    };

    /**
    Constructor.
    */
    inline explicit NonBlockingQueue(const YieldStrategy yieldStrategy);

    /**
    Initializes a user-allocated context as the 'shared' context common to all threads.
    */
    inline void InitializeSharedContext(ContextType *const context);

    /**
    Initializes a user-allocated context as the context associated with the calling thread.
    */
    inline void InitializeWorkerContext(ContextType *const context);

    /**
    Releases a previously initialized shared context.
    */
    inline void ReleaseSharedContext(ContextType *const context);

    /**
    Releases a previously initialized worker thread context.
    */
    inline void ReleaseWorkerContext(ContextType *const context);

    /**
    Resets to zero the given counter for the given thread context.
    */
    inline void ResetCounter(ContextType *const context, const uint32_t counter) const;

    /**
    Gets the value of the given counter for the given thread context.
    */
    inline uint32_t GetCounterValue(const ContextType *const context, const uint32_t counter) const;

    /**
    Returns true if a call to Pop would return no mailbox, for the given context.
    */
    inline bool Empty(const ContextType *const context) const;

    /**
    Returns true if the thread with the given context is still enabled.
    */
    inline bool Running(const ContextType *const context) const;

    /**
    Wakes any worker threads which are blocked waiting for the queue to become non-empty.
    */
    inline void WakeAll();

    /**
    Pushes a mailbox into the queue, scheduling it for processing.
    */
    inline void Push(ContextType *const context, Mailbox *mailbox);

    /**
    Pops a previously pushed mailbox from the queue for processing.
    */
    inline Mailbox *Pop(ContextType *const context);

    /**
    Processes a previously popped mailbox using the provided processor and user context.
    */
    template <class UserContextType, class ProcessorType>
    inline void Process(
        ContextType *const context,
        UserContextType *const userContext,
        Mailbox *const mailbox);

private:

    NonBlockingQueue(const NonBlockingQueue &other);
    NonBlockingQueue &operator=(const NonBlockingQueue &other);

    YieldStrategy mYieldStrategy;
    mutable SpinLock mSharedWorkQueueSpinLock;      ///< Spinlock protecting the shared work queue.
    Queue<Mailbox> mSharedWorkQueue;                ///< Work queue shared by all the threads in a scheduler.
};


inline NonBlockingQueue::NonBlockingQueue(const YieldStrategy yieldStrategy) : mYieldStrategy(yieldStrategy)
{
}


inline void NonBlockingQueue::InitializeSharedContext(ContextType *const context)
{
    context->mShared = true;
}


inline void NonBlockingQueue::InitializeWorkerContext(ContextType *const context)
{
    // Only worker threads should call this method.
    context->mRunning = true;
    context->mShared = false;

    switch (mYieldStrategy)
    {
        default:                        context->mYield.SetYieldFunction(&Detail::YieldPolicy::YieldPolite);       break;
        case YIELD_STRATEGY_POLITE:     context->mYield.SetYieldFunction(&Detail::YieldPolicy::YieldPolite);       break;
        case YIELD_STRATEGY_STRONG:     context->mYield.SetYieldFunction(&Detail::YieldPolicy::YieldStrong);       break;
        case YIELD_STRATEGY_AGGRESSIVE: context->mYield.SetYieldFunction(&Detail::YieldPolicy::YieldAggressive);   break;
    }
}


inline void NonBlockingQueue::ReleaseSharedContext(ContextType *const /*context*/)
{
}


inline void NonBlockingQueue::ReleaseWorkerContext(ContextType *const context)
{
    context->mRunning = false;
}


inline void NonBlockingQueue::ResetCounter(ContextType *const context, const uint32_t counter) const
{
    context->mCounters[counter].mValue.Store(0);
}


inline uint32_t NonBlockingQueue::GetCounterValue(const ContextType *const context, const uint32_t counter) const
{
    return context->mCounters[counter].mValue.Load();
}


THERON_FORCEINLINE bool NonBlockingQueue::Empty(const ContextType *const context) const
{
    // Check the context's local queue.
    // If the provided context is the shared context then it doesn't have a local queue.
    if (!context->mShared && context->mLocalWorkQueue)
    {
        return false;
    }

    // Check the shared work queue.
    bool empty(true);
    mSharedWorkQueueSpinLock.Lock();

    if (!mSharedWorkQueue.Empty())
    {
        empty = false;
    }

    mSharedWorkQueueSpinLock.Unlock();
    return empty;
}


THERON_FORCEINLINE bool NonBlockingQueue::Running(const ContextType *const context) const
{
    return context->mRunning;
}


THERON_FORCEINLINE void NonBlockingQueue::WakeAll()
{
    // Queue implementation is non-blocking, so threads don't block and don't need waking.
}


THERON_FORCEINLINE void NonBlockingQueue::Push(ContextType *const context, Mailbox *mailbox)
{
    // Update the maximum mailbox queue length seen by this thread.
    const uint32_t messageCount(mailbox->Count());
    Atomic::UInt32 &counter(context->mCounters[Theron::COUNTER_MAILBOX_QUEUE_MAX].mValue);

    uint32_t currentValue(counter.Load());
    uint32_t backoff(0);

    while (messageCount > currentValue)
    {
        if (counter.CompareExchangeAcquire(currentValue, messageCount))
        {
            break;
        }

        Utils::Backoff(backoff);
    }

    // Try to push the mailbox onto the local queue of the calling worker thread context.
    // The local queue in a per-thread context is only accessed by that thread
    // so we don't need to protect access to it.
    // The shared context doesn't have a local queue.
    if (!context->mShared)
    {
        // If there's already a mailbox in the local queue then
        // swap it with the new mailbox. Effectively we promote the
        // previously pushed mailbox to the shared queue. This ensures we
        // never have more than one mailbox serialized on the local queue.
        // Promoting the earlier mailbox helps to promote fairness.
        // Also we now know that the earlier mailbox wasn't the last mailbox
        // messaged by this actor, whereas the new one might be. It's best
        // to push to the local queue only the last mailbox messaged by an
        // actor - ideally one messaged right at the end or 'tail' of the handler.
        // This constitutes a kind of tail recursion optimization.
        Mailbox *const previous(context->mLocalWorkQueue);
        context->mLocalWorkQueue = mailbox;

        context->mCounters[Theron::COUNTER_LOCAL_PUSHES].mValue.Increment();

        if (previous == 0)
        {
            return;
        }

        mailbox = previous;
    }

    // Push the mailbox onto the shared work queue.
    // Because the shared queue is accessed by multiple threads we have to protect it.
    mSharedWorkQueueSpinLock.Lock();
    mSharedWorkQueue.Push(mailbox);
    mSharedWorkQueueSpinLock.Unlock();

    context->mCounters[Theron::COUNTER_SHARED_PUSHES].mValue.Increment();
}


THERON_FORCEINLINE Mailbox *NonBlockingQueue::Pop(ContextType *const context)
{
    Mailbox *mailbox(0);

    // The shared context is never used to call Pop, only to Push
    // messages sent outside the context of a worker thread.
    THERON_ASSERT(context->mShared == false);

    // Try to pop a mailbox off the calling thread's local work queue.
    // We only check the shared queue once the local queue is empty.
    // Note that the local queue contains at most one item.
    if (context->mLocalWorkQueue)
    {
        mailbox = context->mLocalWorkQueue;
        context->mLocalWorkQueue = 0;

        context->mYield.Reset();
        return mailbox;
    }

    // Pop a mailbox off the shared work queue.
    // Because the shared queue is accessed by multiple threads we have to protect it.
    // In this implementation the shared queue is protected by a spinlock.
    mSharedWorkQueueSpinLock.Lock();
    if (!mSharedWorkQueue.Empty())
    {
        mailbox = static_cast<Mailbox *>(mSharedWorkQueue.Pop());
    }
    mSharedWorkQueueSpinLock.Unlock();

    if (mailbox)
    {
        context->mYield.Reset();
        return mailbox;
    }

    // Progressive backoff.
    context->mCounters[Theron::COUNTER_YIELDS].mValue.Increment();
    context->mYield.Execute();

    return 0;
}


template <class UserContextType, class ProcessorType>
THERON_FORCEINLINE void NonBlockingQueue::Process(
    ContextType *const context,
    UserContextType *const userContext,
    Mailbox *const mailbox)
{
    // The shared context is never used to call Process.
    THERON_ASSERT(context->mShared == false);

    // Increment the context's message processing event counter.
    context->mCounters[Theron::COUNTER_MESSAGES_PROCESSED].mValue.Increment();
    ProcessorType::Process(userContext, mailbox);
}


} // namespace Detail
} // namespace Theron


#ifdef _MSC_VER
#pragma warning(pop)
#endif //_MSC_VER


#endif // THERON_DETAIL_SCHEDULER_NONBLOCKINGQUEUE_H
