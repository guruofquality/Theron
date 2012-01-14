// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_DETAIL_THREADPOOL_THREADPOOL_H
#define THERON_DETAIL_THREADPOOL_THREADPOOL_H


#include <Theron/Detail/BasicTypes.h>
#include <Theron/Detail/Containers/IntrusiveQueue.h>
#include <Theron/Detail/Core/ActorCore.h>
#include <Theron/Detail/Core/ActorDestroyer.h>
#include <Theron/Detail/Debug/Assert.h>
#include <Theron/Detail/Directory/Directory.h>
#include <Theron/Detail/Messages/IMessage.h>
#include <Theron/Detail/Messages/MessageCreator.h>
#include <Theron/Detail/Threading/Lock.h>
#include <Theron/Detail/Threading/Thread.h>
#include <Theron/Detail/Threading/Monitor.h>
#include <Theron/Detail/ThreadPool/ThreadCollection.h>

#include <Theron/Align.h>
#include <Theron/AllocatorManager.h>
#include <Theron/Defines.h>


namespace Theron
{
namespace Detail
{


/// A pool of worker threads.
class ThreadPool
{
public:

    /// Worker thread entry point function.
    /// Only global (static) functions can be used as thread entry points. Therefore this static method
    /// exists to wrap the non-static class method that is the real entry point.
    /// \param context Pointer to a context class that provides the context in which the thread is run.
    static void StaticWorkerThreadEntryPoint(void *const context);

    /// Manager thread entry point function.
    static void StaticManagerThreadEntryPoint(void *const context);

    /// Constructor.
    ThreadPool();

    /// Starts the pool, starting the given number of worker threads.
    void Start(const uint32_t count);

    /// Stops the pool, terminating all worker threads.
    void Stop();

    /// Requests that there be at most \ref count worker threads in the pool.
    /// \note If the current number is higher, threads are terminated until the maximum is reached.
    void SetMaxThreads(const uint32_t count);

    /// Requests that there be at least \ref count worker threads in the pool.
    /// \note If the current number is lower, new threads are spawned until the maximum is reached.
    void SetMinThreads(const uint32_t count);

    /// Returns the current maximum permitted number of worker threads in this pool.
    inline uint32_t GetMaxThreads() const;

    /// Returns the current minimum permitted number of worker threads in this pool.
    inline uint32_t GetMinThreads() const;

    /// Gets the actual number of worker threads currently in the pool.
    inline uint32_t GetNumThreads() const;

    /// Gets the peak number of worker threads ever in the pool.
    /// \note This includes any threads which were created but later terminated.
    inline uint32_t GetPeakThreads() const;

    /// Resets internal counters that track reported events for thread pool management.
    inline void ResetCounters() const;

    /// Returns the number of messages processed within this pool.
    /// The count is incremented automatically and can be reset using ResetCounters.
    inline uint32_t GetNumMessagesProcessed() const;

    /// Returns the number of thread pulse events made in response to arriving messages.
    /// The count is incremented automatically and can be reset using ResetCounters.
    inline uint32_t GetNumThreadsPulsed() const;

    /// Returns the number of threads woken by pulse events in response to arriving messages.
    /// The count is incremented automatically and can be reset using ResetCounters.
    inline uint32_t GetNumThreadsWoken() const;

    /// Gets a reference to the core message processing mutex.
    inline Mutex &GetMutex() const;

    /// Pushes an actor that has received a message onto the work queue for processing,
    /// and wakes up a worker thread to process it if one is available.
    inline void Push(ActorCore *const actor);

    /// Pushes an actor that has received a message onto the work queue for processing,
    /// without waking up a worker thread. Instead the actor is processed by a running thread.
    inline void TailPush(ActorCore *const actor);

private:

    typedef IntrusiveQueue<ActorCore> WorkQueue;

    /// Clamps a given thread count to a legal range.
    inline static uint32_t ClampThreadCount(const uint32_t count);

    ThreadPool(const ThreadPool &other);
    ThreadPool &operator=(const ThreadPool &other);

    /// Worker thread function.
    void WorkerThreadProc();

    /// Manager thread function.
    void ManagerThreadProc();

    /// Processes an actor core entry retrieved from the work queue.
    inline void ProcessActorCore(Lock &lock, ActorCore *const actorCore);

    // Accessed in the main loop.
    uint32_t mNumThreads;                       ///< Counts the number of threads running.
    uint32_t mTargetThreads;                    ///< The number of threads currently desired.
    WorkQueue mWorkQueue;                       ///< Threadsafe queue of actors waiting to be processed.
    mutable Monitor mWorkQueueMonitor;          ///< Synchronizes access to the work queue.
    mutable Monitor mManagerMonitor;            ///< Locking event that wakes the manager thread.
    mutable uint32_t mNumMessagesProcessed;     ///< Counter used to count processed messages.
    mutable uint32_t mNumThreadsPulsed;         ///< Counts the number of times we signaled a worker thread to wake.
    mutable uint32_t mNumThreadsWoken;          ///< Counter used to count woken threads.

    // Accessed infrequently.
    ThreadCollection mWorkerThreads;            ///< Owned collection of worker threads.
    Thread mManagerThread;                      ///< Dynamically creates and destroys the worker threads.
};


THERON_FORCEINLINE uint32_t ThreadPool::GetMaxThreads() const
{
    uint32_t count(0);

    {
        Lock lock(mManagerMonitor.GetMutex());
        count = mTargetThreads;
    }

    return count;
}


THERON_FORCEINLINE uint32_t ThreadPool::GetMinThreads() const
{
    uint32_t count(0);

    {
        Lock lock(mManagerMonitor.GetMutex());
        count = mTargetThreads;
    }

    return count;
}


THERON_FORCEINLINE uint32_t ThreadPool::GetNumThreads() const
{
    uint32_t count(0);

    {
        Lock lock(mManagerMonitor.GetMutex());
        count = mNumThreads;
    }

    return count;
}


THERON_FORCEINLINE uint32_t ThreadPool::GetPeakThreads() const
{
    uint32_t count(0);

    {
        Lock lock(mManagerMonitor.GetMutex());
        count = mWorkerThreads.Size();
    }

    return count;
}


THERON_FORCEINLINE void ThreadPool::ResetCounters() const
{
    Lock lock(mWorkQueueMonitor.GetMutex());

    mNumMessagesProcessed = 0;
    mNumThreadsPulsed = 0;
    mNumThreadsWoken = 0;
}


THERON_FORCEINLINE uint32_t ThreadPool::GetNumMessagesProcessed() const
{
    uint32_t count(0);

    {
        Lock lock(mWorkQueueMonitor.GetMutex());
        count = mNumMessagesProcessed;
    }

    return count;
}


THERON_FORCEINLINE uint32_t ThreadPool::GetNumThreadsPulsed() const
{
    uint32_t count(0);

    {
        Lock lock(mWorkQueueMonitor.GetMutex());
        count = mNumThreadsPulsed;
    }

    return count;
}


THERON_FORCEINLINE uint32_t ThreadPool::GetNumThreadsWoken() const
{
    uint32_t count(0);

    {
        Lock lock(mWorkQueueMonitor.GetMutex());
        count = mNumThreadsWoken;
    }

    return count;
}


THERON_FORCEINLINE uint32_t ThreadPool::ClampThreadCount(const uint32_t count)
{
    if (count == 0)
    {
        return 1;
    }

    if (count > THERON_MAX_THREADS_PER_FRAMEWORK)
    {
        return THERON_MAX_THREADS_PER_FRAMEWORK;
    }

    return count;
}


THERON_FORCEINLINE Mutex &ThreadPool::GetMutex() const
{
    return mWorkQueueMonitor.GetMutex();
}


THERON_FORCEINLINE void ThreadPool::Push(ActorCore *const actorCore)
{
    // Don't schedule ourselves if the actor is already scheduled or running.
    // Actors which need further processing at the end of their current
    // processing, are marked dirty, and will be rescheduled when executed.
    if (actorCore->IsScheduled())
    {
        // Flag the actor as dirty so it will be re-scheduled after execution.
        actorCore->Dirty();
        return;
    }

    // Mark the actor as busy.
    actorCore->Schedule();

    // Push the actor onto the work queue and wake a worker thread.
    mWorkQueue.Push(actorCore);

    // Wake up a worker thread.
    mWorkQueueMonitor.Pulse();
    ++mNumThreadsPulsed;
}


THERON_FORCEINLINE void ThreadPool::TailPush(ActorCore *const actorCore)
{
    // Don't schedule ourselves if the actor is already scheduled or running.
    // Actors which need further processing at the end of their current
    // processing, are marked dirty, and will be rescheduled when executed.
    if (actorCore->IsScheduled())
    {
        // Flag the actor as dirty so it will be re-scheduled after execution.
        actorCore->Dirty();
        return;
    }

    // Mark the actor as busy.
    actorCore->Schedule();

    // Push the actor onto the work queue without waking a worker thread.
    mWorkQueue.Push(actorCore);
}


THERON_FORCEINLINE void ThreadPool::ProcessActorCore(Lock &lock, ActorCore *const actorCore)
{
    // Read an unprocessed message from the actor's message queue.
    // If there are no queued messages the returned pointer is null.
    IMessage *const message(actorCore->GetQueuedMessage());

    // We have to hold the lock while we check the referenced state, to make sure a
    // dereferencing ActorRef that decremented the reference count has finished accessing
    // it before we free it, in the case where the actor has become unreferenced.
    const bool referenced(actorCore->IsReferenced());

    // Increment the message processing counter if we'll process a message.
    // We do this while still holding the lock to ensure the counter can't be cleared
    // just before we increment it. We exploit the fact that bools are 0 or 1 to avoid branches.
    const uint32_t messageValid(message != 0);
    const uint32_t actorReferenced(referenced);
    mNumMessagesProcessed += (messageValid & actorReferenced);

    lock.Unlock();

    // An actor is still 'live' if it has unprocessed messages or is still referenced.
    const bool live((message != 0) | referenced);
    if (live)
    {
        // If the actor has a waiting message then process the message, even if the
        // actor is no longer referenced. This ensures messages send to actors just
        // before they become unreferenced are correctly processed.
        if (message)
        {
            // Update the actor's message handlers and handle the message.
            actorCore->ValidateHandlers();
            actorCore->ProcessMessage(message);

            // Destroy the message now it's been read.
            // The directory lock is used to protect the global free list.
            Lock directoryLock(Directory::GetMutex());
            MessageCreator::Destroy(message);
        }
    }
    else
    {
        // Garbage collect the unreferenced actor.
        // This also frees any messages still in its queue.
        ActorDestroyer::DestroyActor(actorCore);
    }

    lock.Relock();

    if (live)
    {
        // Re-add the actor to the work queue if it still needs more processing,
        // including if it's unreferenced and we haven't destroyed it yet.
        if (actorCore->IsDirty() | actorCore->HasQueuedMessage() | !referenced)
        {
            actorCore->CleanAndSchedule();
            mWorkQueue.Push(actorCore);
            return;
        }

        actorCore->CleanAndUnschedule();
    }
}


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_THREADPOOL_THREADPOOL_H

