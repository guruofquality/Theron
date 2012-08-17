// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_DETAIL_MAILBOXPROCESSOR_THREADPOOL_H
#define THERON_DETAIL_MAILBOXPROCESSOR_THREADPOOL_H


#include <new>

#include <Theron/Align.h>
#include <Theron/Assert.h>
#include <Theron/BasicTypes.h>
#include <Theron/Defines.h>

#include <Theron/Detail/Allocators/PoolAllocator.h>
#include <Theron/Detail/MailboxProcessor/ThreadCollection.h>
#include <Theron/Detail/Threading/SpinLock.h>
#include <Theron/Detail/Threading/Thread.h>
#include <Theron/Detail/Threading/Utils.h>


namespace Theron
{
namespace Detail
{


/**
A pool of worker threads that process a queue of work items.
*/
template <class WorkQueue, class WorkProcessor, class WorkerContext>
class ThreadPool
{
public:

    /**
    Worker thread entry point function.
    Only global (static) functions can be used as thread entry points. Therefore this static method
    exists to wrap the non-static class method that is the real entry point.
    \param context Pointer to a context class that provides the context in which the thread is run.
    */
    static void StaticWorkerThreadEntryPoint(void *const context);

    /**
    Manager thread entry point function.
    */
    static void StaticManagerThreadEntryPoint(void *const context);

    /**
    Constructor.
    */
    explicit ThreadPool(WorkQueue &workQueue);

    /**
    Destructor.
    */
    ~ThreadPool();

    /**
    Returns the number of worker threads currently in existence.
    */
    inline uint32_t GetThreadCount() const;

    /**
    Creates an additional worker thread for processing of work items.
    \param workerContext Pointer to a caller-owned context scratch pad for the thread.
    \param nodeMask Bitmask specifying on which NUMA processor nodes the thread may execute.
    \param processorMask Bitmask specifying a subset of the processors in each indicated NUMA processor node.
    */
    inline bool CreateThread(
        WorkerContext *const workerContext,
        const uint32_t nodeMask,
        const uint32_t processorMask);

    /**
    Destroys one of the running worker threads and returns its context pointer.
    \return Pointer to the caller-owned context scratch pad for the thread.
    */
    inline WorkerContext *DestroyThread();

    /**
    Destroys all of the running worker threads.
    */
    inline bool DestroyAllThreads();

    /**
    Returns true if the threadpool has no work in its queue.
    */
    inline bool Empty() const;

private:

    /**
    Wrapper struct used to pass both a threadpool pointer and a per-thread context to a started thread.
    */
    struct ThreadContext
    {
        inline ThreadContext(ThreadPool *const threadPool, WorkerContext *const workerContext) :
          mThreadPool(threadPool),
          mWorkerContext(workerContext)
        {
        }

        ThreadPool *mThreadPool;                ///< Used to execute a member function entry point.
        WorkerContext *mWorkerContext;          ///< Pointer to per-thread storage for a worker thread.
    };

    typedef PoolAllocator<ThreadContext, THERON_CACHELINE_ALIGNMENT> ContextAllocator;

    ThreadPool(const ThreadPool &other);
    ThreadPool &operator=(const ThreadPool &other);

    inline void WorkerThreadProc(WorkerContext *const workerContext);
    inline void ManagerThreadProc();

    mutable SpinLock mSpinLock;                 ///< Protects access to shared internal state.
    WorkQueue *mWorkQueue;                      ///< Pointer to a queue of work items for processing.
    uint32_t mThreadCount;                      ///< The number of threads currently running.
    ThreadCollection mWorkerThreads;            ///< Owned collection of worker threads.
    ContextAllocator mContextAllocator;         ///< Caching pool allocator for thread contexts.
    Thread mManagerThread;                      ///< Dynamically creates and destroys the worker threads.
    bool mDone;                                 ///< Tells the threads to terminate prior to destruction.
};


template <class WorkQueue, class WorkProcessor, class WorkerContext>
inline ThreadPool<WorkQueue, WorkProcessor, WorkerContext>::ThreadPool(WorkQueue &workQueue) :
  mSpinLock(),
  mWorkQueue(&workQueue),
  mThreadCount(0),
  mWorkerThreads(),
  mContextAllocator(),
  mManagerThread(),
  mDone(false)
{
    // Start the manager thread.
    mManagerThread.Start(StaticManagerThreadEntryPoint, this);
}


template <class WorkQueue, class WorkProcessor, class WorkerContext>
inline ThreadPool<WorkQueue, WorkProcessor, WorkerContext>::~ThreadPool()
{
    // Wait for the manager thread to terminate.
    mManagerThread.Join();
}


template <class WorkQueue, class WorkProcessor, class WorkerContext>
inline uint32_t ThreadPool<WorkQueue, WorkProcessor, WorkerContext>::GetThreadCount() const
{
    uint32_t threadCount(0);

    // Lock the manager thread monitor while we access shared state.
    mSpinLock.Lock();
    threadCount = mThreadCount;
    mSpinLock.Unlock();

    return threadCount;
}


template <class WorkQueue, class WorkProcessor, class WorkerContext>
inline bool ThreadPool<WorkQueue, WorkProcessor, WorkerContext>::CreateThread(
    WorkerContext *const workerContext,
    const uint32_t nodeMask,
    const uint32_t processorMask)
{
    bool success(false);

    mSpinLock.Lock();

    // Use the pool to allocate a context structure for the new thread.
    if (void *const threadContextMemory = mContextAllocator.Allocate(sizeof(ThreadContext)))
    {
        // In-place construct the thread context in the allocated memory.
        ThreadContext *const threadContext(new (threadContextMemory) ThreadContext(this, workerContext));

        // Create the worker thread, passing it the constructed context.
        mWorkerThreads.CreateThread(StaticWorkerThreadEntryPoint, threadContext, nodeMask, processorMask);

        success = true;
    }

    mSpinLock.Unlock();

    return success;
}


template <class WorkQueue, class WorkProcessor, class WorkerContext>
inline WorkerContext *ThreadPool<WorkQueue, WorkProcessor, WorkerContext>::DestroyThread()
{
    // TODO: Not implemented yet.
    return 0;
}


template <class WorkQueue, class WorkProcessor, class WorkerContext>
inline bool ThreadPool<WorkQueue, WorkProcessor, WorkerContext>::DestroyAllThreads()
{
    // Signal the threads to terminate. We're the only writer so synchronization isn't needed.
    mDone = true;

    // Wait for the worker threads to stop, and then delete them.
    mWorkerThreads.DestroyThreads();

    return true;
}


template <class WorkQueue, class WorkProcessor, class WorkerContext>
inline bool ThreadPool<WorkQueue, WorkProcessor, WorkerContext>::Empty() const
{
    return mWorkQueue->Empty();
}


template <class WorkQueue, class WorkProcessor, class WorkerContext>
inline void ThreadPool<WorkQueue, WorkProcessor, WorkerContext>::StaticWorkerThreadEntryPoint(void *const context)
{
    // The static entry point function is provided with a pointer to a context structure.
    // The context structure is specific to this worker thread.
    THERON_ASSERT(context);
    ThreadContext *const threadContext(reinterpret_cast<ThreadContext *>(context));

    THERON_ASSERT(threadContext->mThreadPool);
    THERON_ASSERT(threadContext->mWorkerContext);

    // The thread entry point has to be a static function,
    // so in this static wrapper function we call the non-static method
    // on the instance, a pointer to which is provided in the context structure.
    ThreadPool *const threadPool(threadContext->mThreadPool);
    WorkerContext *const workerContext(threadContext->mWorkerContext);

    // Call the member function entry point, passing it the thread's context storage.
    threadPool->WorkerThreadProc(workerContext);

    // Destruct the thread context.
    threadContext->~ThreadContext();

    // Lock the manager thread monitor while we access shared state.
    threadPool->mSpinLock.Lock();
    threadPool->mContextAllocator.Free(threadContext);
    threadPool->mSpinLock.Unlock();
}


template <class WorkQueue, class WorkProcessor, class WorkerContext>
inline void ThreadPool<WorkQueue, WorkProcessor, WorkerContext>::StaticManagerThreadEntryPoint(void *const context)
{
    ThreadPool *const threadPool(reinterpret_cast<ThreadPool *>(context));
    threadPool->ManagerThreadProc();
}


template <class WorkQueue, class WorkProcessor, class WorkerContext>
inline void ThreadPool<WorkQueue, WorkProcessor, WorkerContext>::WorkerThreadProc(WorkerContext *const context)
{
    // Lock the manager thread monitor while we access shared state.
    mSpinLock.Lock();
    ++mThreadCount;
    mSpinLock.Unlock();

    uint32_t backoff(0);
    while (!mDone)
    {
        // Try to get a work item from the work queue.
        if (typename WorkQueue::ItemType *const item = mWorkQueue->Pop())
        {
            // Process the item and re-schedule it if it needs more processing.
            WorkProcessor::Process(item, &context->mProcessorContext);
            backoff = 0;
        }
        else
        {
            Utils::Backoff(backoff);
        }
    }

    // Lock the manager thread monitor while we access shared state.
    mSpinLock.Lock();
    --mThreadCount;
    mSpinLock.Unlock();
}


template <class WorkQueue, class WorkProcessor, class WorkerContext>
inline void ThreadPool<WorkQueue, WorkProcessor, WorkerContext>::ManagerThreadProc()
{
    // TODO: Currently the manager thread isn't used.
    while (!mDone)
    {
        Utils::SleepThread(10);
    }
}


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_MAILBOXPROCESSOR_THREADPOOL_H
