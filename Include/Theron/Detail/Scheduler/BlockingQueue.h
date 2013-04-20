// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_DETAIL_SCHEDULER_BLOCKINGQUEUE_H
#define THERON_DETAIL_SCHEDULER_BLOCKINGQUEUE_H


#include <new>

#include <Theron/AllocatorManager.h>
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
          mShared(false),
          mLocalWorkQueue(),
          mLock(0)
        {
        }

    private:

        bool mShared;                               ///< Indicates whether this is the 'shared' context.
        Queue<Mailbox> mLocalWorkQueue;             ///< Local thread-specific work queue.
        Lock *mLock;                                ///< Pointer to owned lock object.
        Atomic::UInt32 mCounters[MAX_COUNTERS];     ///< Array of per-context event counters.
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
    Wakes any worker threads which are blocked waiting for the queue to become non-empty.
    */
    inline void WakeAll();

    /**
    Pushes a mailbox into the queue, scheduling it for processing.
    \param localThread A hint indicating that the mailbox should be processed by the same thread.
    */
    inline void Push(ContextType *const context, Mailbox *const mailbox, const bool localThread);

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

    // Each worker thread holds the lock almost all the time in the main loop,
    // releasing it only when it is doing actual processing. This protects against
    // weird thread waiting and termination race conditions.
    IAllocator *const allocator(AllocatorManager::Instance().GetAllocator());
    void *const memory = allocator->AllocateAligned(sizeof(Lock), THERON_CACHELINE_ALIGNMENT);

    THERON_ASSERT_MSG(memory, "Failed to allocate lock for queue context");
    context->mLock = new (memory) Lock(mSharedWorkQueueCondition.GetMutex());
}


inline void BlockingQueue::ReleaseSharedContext(ContextType *const /*context*/)
{
}


inline void BlockingQueue::ReleaseWorkerContext(ContextType *const context)
{
    // Each worker thread finally releases the lock it holds on the queue mutex.
    context->mLock->~Lock();

    IAllocator *const allocator(AllocatorManager::Instance().GetAllocator());
    allocator->Free(context->mLock, sizeof(Lock));
    context->mLock = 0;
}


inline void BlockingQueue::ResetCounter(ContextType *const context, const uint32_t counter) const
{
    context->mCounters[counter].Store(0);
}


inline uint32_t BlockingQueue::GetCounterValue(const ContextType *const context, const uint32_t counter) const
{
    return context->mCounters[counter].Load();
}


THERON_FORCEINLINE bool BlockingQueue::Empty(const ContextType *const context) const
{
    // Check the context's local queue.
    // If the provided context is the shared context then it doesn't have a local queue.
    if (!context->mShared)
    {
        if (!context->mLocalWorkQueue.Empty())
        {
            return false;
        }
    }

    // Check the shared work queue.
    Lock lock(mSharedWorkQueueCondition.GetMutex());
    return mSharedWorkQueue.Empty();
}


THERON_FORCEINLINE void BlockingQueue::WakeAll()
{
    mSharedWorkQueueCondition.PulseAll();
}


THERON_FORCEINLINE void BlockingQueue::Push(
    ContextType *const context,
    Mailbox *const mailbox,
    const bool localThread)
{
    // Try to push the mailbox onto the local queue of the calling worker thread context.
    // The shared context doesn't have a local queue.
    if (localThread && !context->mShared)
    {
        // The local queue in a per-thread context is only accessed by that thread
        // so we don't need to protect access to it.
        context->mLocalWorkQueue.Push(mailbox);
        context->mCounters[Theron::COUNTER_LOCAL_PUSHES].Increment();

        return;
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
    context->mCounters[Theron::COUNTER_SHARED_PUSHES].Increment();
}


THERON_FORCEINLINE Mailbox *BlockingQueue::Pop(ContextType *const context)
{
    // The shared context is never used to call Pop, only to Push
    // messages sent outside the context of a worker thread.
    THERON_ASSERT(context->mShared == false);

    // Try to pop a mailbox off the calling thread's local work queue.
    // We only check the shared queue once the local queue is empty.
    if (!context->mLocalWorkQueue.Empty())
    {
        return static_cast<Mailbox *>(context->mLocalWorkQueue.Pop());
    }

    // Pop a mailbox off the shared work queue.
    // Because the shared queue is accessed by multiple threads we have to protect it.
    // The calling worker thread context contains a lock on the mutex which should currently be locked.
    // We unlock it only temporarily while processing popped mailboxes.
    if (!mSharedWorkQueue.Empty())
    {
        return static_cast<Mailbox *>(mSharedWorkQueue.Pop());
    }

    // Wait to be pulsed when work arrives on the shared queue.
    context->mCounters[Theron::COUNTER_YIELDS].Increment();
    mSharedWorkQueueCondition.Wait(*context->mLock);

    return 0;
}


template <class UserContextType, class ProcessorType>
THERON_FORCEINLINE void BlockingQueue::Process(
    ContextType *const context,
    UserContextType *const userContext,
    Mailbox *const mailbox)
{
    // The shared context is never used to call Process.
    THERON_ASSERT(context->mShared == false);
    THERON_ASSERT(context->mLock != 0);

    // We unlock the lock on the shared queue mutex while we process the item.
    context->mLock->Unlock();

    // Increment the context's message processing event counter.
    context->mCounters[Theron::COUNTER_MESSAGES_PROCESSED].Increment();
    ProcessorType::Process(userContext, mailbox);

    context->mLock->Relock();
}


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_SCHEDULER_BLOCKINGQUEUE_H
