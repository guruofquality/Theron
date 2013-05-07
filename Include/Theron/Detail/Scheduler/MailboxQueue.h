// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_DETAIL_SCHEDULER_MAILBOXQUEUE_H
#define THERON_DETAIL_SCHEDULER_MAILBOXQUEUE_H


#include <Theron/Align.h>
#include <Theron/Assert.h>
#include <Theron/BasicTypes.h>
#include <Theron/Counters.h>
#include <Theron/Defines.h>
#include <Theron/YieldStrategy.h>

#include <Theron/Detail/Containers/Queue.h>
#include <Theron/Detail/Mailboxes/Mailbox.h>
#include <Theron/Detail/Scheduler/Counting.h>
#include <Theron/Detail/Threading/Atomic.h>
#include <Theron/Detail/Threading/Clock.h>
#include <Theron/Detail/Threading/Utils.h>


#ifdef _MSC_VER
#pragma warning(push)
#pragma warning (disable:4324)  // structure was padded due to __declspec(align())
#endif //_MSC_VER


namespace Theron
{
namespace Detail
{


/**
\brief Generic mailbox queue implementation with specialized per-thread local queues.
*/
template <class MonitorType>
class MailboxQueue
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

        friend class MailboxQueue;

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
        typename MonitorType::Context mMonitorContext;      ///< Per-thread monitor primitive context.
        Aligned<Atomic::UInt32> mCounters[MAX_COUNTERS];    ///< Array of per-context event counters.
    };

    /**
    Constructor.
    */
    inline explicit MailboxQueue(const YieldStrategy yieldStrategy);

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

private:

    MailboxQueue(const MailboxQueue &other);
    MailboxQueue &operator=(const MailboxQueue &other);

    mutable MonitorType mMonitor;           ///< Synchronizes access to the shared queue.
    Queue<Mailbox> mSharedWorkQueue;        ///< Work queue shared by all the threads in a scheduler.
};


template <class MonitorType>
inline MailboxQueue<MonitorType>::MailboxQueue(const YieldStrategy yieldStrategy) : mMonitor(yieldStrategy)
{
}


template <class MonitorType>
inline void MailboxQueue<MonitorType>::InitializeSharedContext(ContextType *const context)
{
    context->mShared = true;
}


template <class MonitorType>
inline void MailboxQueue<MonitorType>::InitializeWorkerContext(ContextType *const context)
{
    // Only worker threads should call this method.
    context->mShared = false;
    context->mRunning = true;

    mMonitor.InitializeWorkerContext(&context->mMonitorContext);
}


template <class MonitorType>
inline void MailboxQueue<MonitorType>::ReleaseSharedContext(ContextType *const /*context*/)
{
}


template <class MonitorType>
inline void MailboxQueue<MonitorType>::ReleaseWorkerContext(ContextType *const context)
{
    typename MonitorType::LockType lock(mMonitor);
    context->mRunning = false;
}


template <class MonitorType>
inline void MailboxQueue<MonitorType>::ResetCounter(ContextType *const context, const uint32_t counter) const
{
    THERON_COUNTER_RESET(context->mCounters[counter].mValue);
}


template <class MonitorType>
THERON_FORCEINLINE uint32_t MailboxQueue<MonitorType>::GetCounterValue(const ContextType *const context, const uint32_t counter) const
{
    return THERON_COUNTER_QUERY(context->mCounters[counter].mValue);
}


template <class MonitorType>
THERON_FORCEINLINE bool MailboxQueue<MonitorType>::Empty(const ContextType *const context) const
{
    // Check the context's local queue.
    // If the provided context is the shared context then it doesn't have a local queue.
    if (!context->mShared && context->mLocalWorkQueue)
    {
        return false;
    }

    // Check the shared work queue.
    typename MonitorType::LockType lock(mMonitor);
    return mSharedWorkQueue.Empty();
}


template <class MonitorType>
THERON_FORCEINLINE bool MailboxQueue<MonitorType>::Running(const ContextType *const context) const
{
    return context->mRunning;
}


template <class MonitorType>
THERON_FORCEINLINE void MailboxQueue<MonitorType>::WakeAll()
{
    mMonitor.PulseAll();
}


template <class MonitorType>
THERON_FORCEINLINE void MailboxQueue<MonitorType>::Push(ContextType *const context, Mailbox *mailbox)
{
    // Update the maximum mailbox queue length seen by this thread.
    THERON_COUNTER_RAISE(context->mCounters[Theron::COUNTER_MAILBOX_QUEUE_MAX].mValue, mailbox->Count());

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

        THERON_COUNTER_INCREMENT(context->mCounters[Theron::COUNTER_LOCAL_PUSHES].mValue);

        if (previous == 0)
        {
            return;
        }

        mailbox = previous;
    }

    // Push the mailbox onto the shared work queue.
    // Because the shared queue is accessed by multiple threads we have to protect it.
    {
        typename MonitorType::LockType lock(mMonitor);
        mSharedWorkQueue.Push(mailbox);
    }

    // Pulse the condition associated with the shared queue to wake a worker thread.
    // It's okay to release the lock before calling Pulse.
    mMonitor.Pulse();
    THERON_COUNTER_INCREMENT(context->mCounters[Theron::COUNTER_SHARED_PUSHES].mValue);
}


template <class MonitorType>
THERON_FORCEINLINE Mailbox *MailboxQueue<MonitorType>::Pop(ContextType *const context)
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
    }
    else
    {
        // Wait on the shared queue until we pop a mailbox from it.
        // Because the shared queue is accessed by multiple threads we have to protect it.
        typename MonitorType::LockType lock(mMonitor);
        while (mSharedWorkQueue.Empty() && context->mRunning == true)
        {
            THERON_COUNTER_INCREMENT(context->mCounters[Theron::COUNTER_YIELDS].mValue);
            mMonitor.Wait(&context->mMonitorContext, lock);
        }

        if (!mSharedWorkQueue.Empty())
        {
            mailbox = static_cast<Mailbox *>(mSharedWorkQueue.Pop());
            mMonitor.ResetYield(&context->mMonitorContext);
        }
    }

    if (mailbox)
    {
        THERON_COUNTER_INCREMENT(context->mCounters[Theron::COUNTER_MESSAGES_PROCESSED].mValue);
    }

    return mailbox;
}


} // namespace Detail
} // namespace Theron


#ifdef _MSC_VER
#pragma warning(pop)
#endif //_MSC_VER


#endif // THERON_DETAIL_SCHEDULER_MAILBOXQUEUE_H
