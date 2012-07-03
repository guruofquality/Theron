// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_DETAIL_THREADPOOL_THREADPOOL_H
#define THERON_DETAIL_THREADPOOL_THREADPOOL_H


#include <Theron/Align.h>
#include <Theron/BasicTypes.h>
#include <Theron/Defines.h>

#include <Theron/Detail/Containers/IntrusiveList.h>
#include <Theron/Detail/Containers/IntrusiveQueue.h>
#include <Theron/Detail/Core/ActorCore.h>
#include <Theron/Detail/Debug/Assert.h>
#include <Theron/Detail/Threading/Lock.h>
#include <Theron/Detail/Threading/Thread.h>
#include <Theron/Detail/Threading/Monitor.h>
#include <Theron/Detail/ThreadPool/ThreadCollection.h>
#include <Theron/Detail/ThreadPool/WorkerContext.h>


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
    void ResetCounters() const;

    /// Returns the number of messages processed within this pool.
    /// The count is incremented automatically and can be reset using ResetCounters.
    uint32_t GetNumMessagesProcessed() const;

    /// Returns the number of thread pulse events made in response to arriving messages.
    /// The count is incremented automatically and can be reset using ResetCounters.
    uint32_t GetNumThreadsPulsed() const;

    /// Returns the number of threads woken by pulse events in response to arriving messages.
    /// The count is incremented automatically and can be reset using ResetCounters.
    uint32_t GetNumThreadsWoken() const;

    /// Gets a pointer the pulse counter owned by the threadpool.
    inline uint32_t *GetPulseCounterAddress() const;

    /// Gets a reference to the core message processing mutex.
    inline Mutex &GetMutex() const;

    /// Pushes an actor that has received a message onto the work queue for processing,
    /// and wakes up a worker thread to process it if one is available.
    /// \return True, if the threadpool was pulsed to wake a worker thread.
    inline bool Push(ActorCore *const actor);

    /// Pushes an actor that has received a message onto the work queue for processing,
    /// without waking up a worker thread. Instead the actor is processed by a running thread.
    inline void TailPush(ActorCore *const actor);

private:

    typedef IntrusiveQueue<ActorCore> WorkQueue;
    typedef IntrusiveList<WorkerContext> WorkerContextList;

    /// Clamps a given thread count to a legal range.
    inline static uint32_t ClampThreadCount(const uint32_t count);

    ThreadPool(const ThreadPool &other);
    ThreadPool &operator=(const ThreadPool &other);

    /// Worker thread function.
    void WorkerThreadProc(WorkerContext *const workerContext);

    /// Manager thread function.
    void ManagerThreadProc();

    // Accessed in the main loop.
    uint32_t mThreadCount;                      ///< The number of threads currently running.
    uint32_t mTargetThreadCount;                ///< The current target thread count.
    WorkQueue mWorkQueue;                       ///< Threadsafe queue of actors waiting to be processed.
    mutable Monitor mWorkQueueMonitor;          ///< Synchronizes access to the work queue.
    mutable Monitor mManagerMonitor;            ///< Locking event that wakes the manager thread.
    mutable uint32_t mPulseCount;               ///< Number of times we pulsed the threadpool to wake a worker thread.

    // Accessed infrequently.
    ThreadCollection mWorkerThreads;            ///< Owned collection of worker threads.
    WorkerContextList mWorkerContexts;          ///< List of owned worker context structures.
    Thread mManagerThread;                      ///< Dynamically creates and destroys the worker threads.
};


THERON_FORCEINLINE uint32_t ThreadPool::GetMaxThreads() const
{
    uint32_t count(0);

    {
        Lock lock(mManagerMonitor.GetMutex());
        count = mTargetThreadCount;
    }

    return count;
}


THERON_FORCEINLINE uint32_t ThreadPool::GetMinThreads() const
{
    uint32_t count(0);

    {
        Lock lock(mManagerMonitor.GetMutex());
        count = mTargetThreadCount;
    }

    return count;
}


THERON_FORCEINLINE uint32_t ThreadPool::GetNumThreads() const
{
    uint32_t count(0);

    {
        Lock lock(mManagerMonitor.GetMutex());
        count = mThreadCount;
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


THERON_FORCEINLINE uint32_t *ThreadPool::GetPulseCounterAddress() const
{
    return &mPulseCount;
}


THERON_FORCEINLINE Mutex &ThreadPool::GetMutex() const
{
    return mWorkQueueMonitor.GetMutex();
}


THERON_FORCEINLINE bool ThreadPool::Push(ActorCore *const actorCore)
{
    // Don't schedule ourselves if the actor is already scheduled or running.
    // Actors which need further processing at the end of their current
    // processing, are marked dirty, and will be rescheduled when executed.
    if (actorCore->IsScheduled())
    {
        // Flag the actor as dirty so it will be re-scheduled after execution.
        actorCore->Dirty();
        return false;
    }

    // Mark the actor as busy.
    actorCore->Schedule();

    // Push the actor onto the work queue and wake a worker thread.
    mWorkQueue.Push(actorCore);
    mWorkQueueMonitor.Pulse();

    return true;
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


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_THREADPOOL_THREADPOOL_H

