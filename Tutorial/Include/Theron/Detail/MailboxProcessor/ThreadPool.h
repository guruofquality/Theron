// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_DETAIL_MAILBOXPROCESSOR_THREADPOOL_H
#define THERON_DETAIL_MAILBOXPROCESSOR_THREADPOOL_H


#include <new>

#include <Theron/Align.h>
#include <Theron/Assert.h>
#include <Theron/BasicTypes.h>
#include <Theron/Defines.h>

#include <Theron/Detail/Containers/List.h>
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
    User-allocated per-thread context structure.
    The client must allocate one of these for each thread it creates.
    The context structure derives from List<ThreadContext>::Node so can be stored in lists.
    */
    struct ThreadContext : public List<ThreadContext>::Node
    {
    public:

        /**
        Constructor. Creates a ThreadContext wrapping a pointer to user-defined worker context object.
        */
        inline explicit ThreadContext(WorkerContext *const workerContext) :
          mWorkerContext(workerContext),
          mWorkQueue(0),
          mNodeMask(0),
          mProcessorMask(0),
          mRunning(false),
          mStarted(false),
          mThread(0)
        {
        }

        // Public
        WorkerContext *mWorkerContext;          ///< Pointer to user-defined context object.

        // Internal
        WorkQueue *mWorkQueue;                  ///< Pointer to the work queue processed by the thread.
        uint32_t mNodeMask;                     ///< Bit-field NUMA node affinity mask for the created thread.
        uint32_t mProcessorMask;                ///< Bit-field processor affinity mask within specified nodes.
        bool mRunning;                          ///< Indicates whether the thread is running; used to stop running threads.
        bool mStarted;                          ///< Indicates whether the thread has started.
        Thread *mThread;                        ///< Pointer to the thread object.
    };

    /**
    Creates an additional worker thread for processing of work items.
    The thread is created but not yet started - call StartThread for that.
    \param threadContext Pointer to a caller-allocated context object for the thread.
    */
    inline static bool CreateThread(ThreadContext *const threadContext);

    /**
    Starts the given thread, which must have been created with CreateThread.
    \param workQueue Pointer to the shared work queue that the thread will service.
    \param nodeMask Bit-mask specifying on which NUMA processor nodes the thread may execute.
    \param processorMask Bit-mask specifying a subset of the processors in each indicated NUMA processor node.
    */
    inline static bool StartThread(
        ThreadContext *const threadContext,
        WorkQueue *const workQueue,
        const uint32_t nodeMask,
        const uint32_t processorMask);

    /**
    Stops the given thread, which must have been started with StartThread.
    */
    inline static bool StopThread(ThreadContext *const threadContext);

    /**
    Destroys the given thread, which must have been stopped with StopThread.
    */
    inline static bool DestroyThread(ThreadContext *const threadContext);

    /**
    Returns true if the given thread has been started but not stopped.
    */
    inline static bool IsRunning(ThreadContext *const threadContext);

    /**
    Returns true if the given thread has started.
    */
    inline static bool IsStarted(ThreadContext *const threadContext);

private:

    ThreadPool(const ThreadPool &other);
    ThreadPool &operator=(const ThreadPool &other);

    /**
    Worker thread entry point function.
    Only global (static) functions can be used as thread entry points.
    \param context Pointer to a context object that provides the context in which the thread is run.
    */
    inline static void ThreadEntryPoint(void *const context);
};


template <class WorkQueue, class WorkProcessor, class WorkerContext>
inline bool ThreadPool<WorkQueue, WorkProcessor, WorkerContext>::CreateThread(ThreadContext *const threadContext)
{
    // Allocate a new thread, aligning the memory to a cache-line boundary to reduce false-sharing of cache-lines.
    void *const threadMemory = AllocatorManager::Instance().GetAllocator()->AllocateAligned(sizeof(Thread), THERON_CACHELINE_ALIGNMENT);
    if (threadMemory == 0)
    {
        return false;
    }

    // Construct the thread object.
    Thread *const thread = new (threadMemory) Thread();

    // Set up the private part of the user-allocated context. We pass in a pointer to the
    // threadpool instance so we can use a member function as the entry point.
    threadContext->mStarted = false;
    threadContext->mThread = thread;
   
    return true;
}


template <class WorkQueue, class WorkProcessor, class WorkerContext>
inline bool ThreadPool<WorkQueue, WorkProcessor, WorkerContext>::StartThread(
    ThreadContext *const threadContext,
    WorkQueue *const workQueue,
    const uint32_t nodeMask,
    const uint32_t processorMask)
{
    THERON_ASSERT(threadContext->mRunning == false);
    THERON_ASSERT(threadContext->mThread);
    THERON_ASSERT(threadContext->mThread->Running() == false);

    threadContext->mWorkQueue = workQueue;
    threadContext->mNodeMask = nodeMask;
    threadContext->mProcessorMask = processorMask;
    threadContext->mRunning = true;

    // Start the thread, running it via a static (non-member function) entry point that wraps the real member function.
    threadContext->mThread->Start(ThreadEntryPoint, threadContext);

    return true;
}


template <class WorkQueue, class WorkProcessor, class WorkerContext>
inline bool ThreadPool<WorkQueue, WorkProcessor, WorkerContext>::StopThread(ThreadContext *const threadContext)
{
    THERON_ASSERT(threadContext->mThread);
    THERON_ASSERT(threadContext->mThread->Running());

    // Reset the enabled flag in the context object for the thread.
    // The thread will terminate once it sees the flag has been reset.
    threadContext->mRunning = false;

    // Wait for the thread to finish.
    threadContext->mThread->Join();

    return true;
}


template <class WorkQueue, class WorkProcessor, class WorkerContext>
inline bool ThreadPool<WorkQueue, WorkProcessor, WorkerContext>::DestroyThread(ThreadContext *const threadContext)
{
    THERON_ASSERT(threadContext->mRunning == false);
    THERON_ASSERT(threadContext->mThread);
    THERON_ASSERT(threadContext->mThread->Running() == false);

    // Destruct the thread object explicitly since we constructed using placement new.
    threadContext->mThread->~Thread();

    // Free the memory for the thread.
    AllocatorManager::Instance().GetAllocator()->Free(threadContext->mThread, sizeof(Thread));
    threadContext->mThread = 0;

    return true;
}


template <class WorkQueue, class WorkProcessor, class WorkerContext>
THERON_FORCEINLINE bool ThreadPool<WorkQueue, WorkProcessor, WorkerContext>::IsRunning(ThreadContext *const threadContext)
{
    return threadContext->mRunning;
}


template <class WorkQueue, class WorkProcessor, class WorkerContext>
THERON_FORCEINLINE bool ThreadPool<WorkQueue, WorkProcessor, WorkerContext>::IsStarted(ThreadContext *const threadContext)
{
    return threadContext->mStarted;
}


template <class WorkQueue, class WorkProcessor, class WorkerContext>
inline void ThreadPool<WorkQueue, WorkProcessor, WorkerContext>::ThreadEntryPoint(void *const context)
{
    // The static entry point function is provided with a pointer to a context structure.
    // The context structure is specific to this worker thread.
    ThreadContext *const threadContext(reinterpret_cast<ThreadContext *>(context));

    // Set the NUMA node and processor affinity for the running thread.
    Utils::SetThreadAffinity(threadContext->mNodeMask, threadContext->mProcessorMask);

    // Mark the thread as started so the caller knows they can start issuing work.
    threadContext->mStarted = true;

    uint32_t backoff(0);
    while (threadContext->mRunning)
    {
        // Try to get a work item from the work queue.
        if (typename WorkQueue::ItemType *const item = threadContext->mWorkQueue->Pop())
        {
            // Process the item, passing it the user context, and re-schedule it if it needs more processing.
            WorkProcessor::Process(item, threadContext->mWorkerContext);
            backoff = 0;
        }
        else
        {
            Utils::Backoff(backoff);
        }
    }
}


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_MAILBOXPROCESSOR_THREADPOOL_H
