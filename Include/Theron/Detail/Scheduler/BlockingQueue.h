// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_DETAIL_SCHEDULER_BLOCKINGQUEUE_H
#define THERON_DETAIL_SCHEDULER_BLOCKINGQUEUE_H


#include <Theron/Align.h>
#include <Theron/Assert.h>
#include <Theron/BasicTypes.h>
#include <Theron/Counters.h>
#include <Theron/Defines.h>
#include <Theron/YieldStrategy.h>

#include <Theron/Detail/Containers/Queue.h>
#include <Theron/Detail/Mailboxes/Mailbox.h>
#include <Theron/Detail/Threading/Atomic.h>
#include <Theron/Detail/Threading/Condition.h>
#include <Theron/Detail/Threading/Lock.h>
#include <Theron/Detail/Threading/Mutex.h>


#ifdef _MSC_VER
#pragma warning(push)
#pragma warning (disable:4324)  // structure was padded due to __declspec(align())
#endif //_MSC_VER


namespace Theron
{
namespace Detail
{


/**
\brief Blocking queue implementation based on a condition variable.
*/
class BlockingQueue
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

        friend class BlockingQueue;

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
        Aligned<Atomic::UInt32> mCounters[MAX_COUNTERS];    ///< Array of per-context event counters.
    };

    /**
    Constructor.
    */
    inline explicit BlockingQueue(const YieldStrategy yieldStrategy);

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

    BlockingQueue(const BlockingQueue &other);
    BlockingQueue &operator=(const BlockingQueue &other);

    mutable Condition mSharedWorkQueueCondition;    ///< Condition variable synchronizing access to the shared queue.
    Queue<Mailbox> mSharedWorkQueue;                ///< Work queue shared by all the threads in a scheduler.
};


inline BlockingQueue::BlockingQueue(const YieldStrategy /*yieldStrategy*/)
{
}


inline void BlockingQueue::InitializeSharedContext(ContextType *const context)
{
    context->mShared = true;
}


inline void BlockingQueue::InitializeWorkerContext(ContextType *const context)
{
    // Only worker threads should call this method.
    context->mShared = false;
    context->mRunning = true;
}


inline void BlockingQueue::ReleaseSharedContext(ContextType *const /*context*/)
{
}


inline void BlockingQueue::ReleaseWorkerContext(ContextType *const context)
{
    Lock (mSharedWorkQueueCondition.GetMutex());
    context->mRunning = false;
}


inline void BlockingQueue::ResetCounter(ContextType *const context, const uint32_t counter) const
{
    context->mCounters[counter].mValue.Store(0);
}


inline uint32_t BlockingQueue::GetCounterValue(const ContextType *const context, const uint32_t counter) const
{
    return context->mCounters[counter].mValue.Load();
}


THERON_FORCEINLINE bool BlockingQueue::Empty(const ContextType *const context) const
{
    // Check the context's local queue.
    // If the provided context is the shared context then it doesn't have a local queue.
    if (!context->mShared && context->mLocalWorkQueue)
    {
        return false;
    }

    // Check the shared work queue.
    Lock lock(mSharedWorkQueueCondition.GetMutex());
    return mSharedWorkQueue.Empty();
}


THERON_FORCEINLINE bool BlockingQueue::Running(const ContextType *const context) const
{
    return context->mRunning;
}


THERON_FORCEINLINE void BlockingQueue::WakeAll()
{
    mSharedWorkQueueCondition.PulseAll();
}


THERON_FORCEINLINE void BlockingQueue::Push(ContextType *const context, Mailbox *mailbox)
{
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
    // We lock the mutex directly since the shared context doesn't have a lock on it.
    mSharedWorkQueueCondition.GetMutex().Lock();
    mSharedWorkQueue.Push(mailbox);
    mSharedWorkQueueCondition.GetMutex().Unlock();

    // Pulse the condition associated with the shared queue to wake a worker thread.
    // It's okay to release the lock before calling Pulse.
    mSharedWorkQueueCondition.Pulse();
    context->mCounters[Theron::COUNTER_SHARED_PUSHES].mValue.Increment();
}


THERON_FORCEINLINE Mailbox *BlockingQueue::Pop(ContextType *const context)
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
        return mailbox;
    }

    // Pop a mailbox off the shared work queue.
    // Because the shared queue is accessed by multiple threads we have to protect it.
    Lock lock(mSharedWorkQueueCondition.GetMutex());
    if (!mSharedWorkQueue.Empty())
    {
        mailbox = static_cast<Mailbox *>(mSharedWorkQueue.Pop());
    }
    else if (context->mRunning == true)
    {
        // Wait to be pulsed when work arrives on the shared queue.
        context->mCounters[Theron::COUNTER_YIELDS].mValue.Increment();
        mSharedWorkQueueCondition.Wait(lock);
    }

    return mailbox;
}


template <class UserContextType, class ProcessorType>
THERON_FORCEINLINE void BlockingQueue::Process(
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


#endif // THERON_DETAIL_SCHEDULER_BLOCKINGQUEUE_H
