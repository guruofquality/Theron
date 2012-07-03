// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#include <Theron/Detail/ThreadPool/ActorProcessor.h>
#include <Theron/Detail/ThreadPool/ThreadPool.h>

#include <Theron/AllocatorManager.h>
#include <Theron/Defines.h>


#ifdef _MSC_VER
#pragma warning(push)
#pragma warning (disable:4127)  // Conditional expression is constant.
#endif //_MSC_VER


namespace Theron
{
namespace Detail
{


ThreadPool::ThreadPool() :
  mThreadCount(0),
  mTargetThreadCount(0),
  mWorkQueue(),
  mWorkQueueMonitor(),
  mManagerMonitor(),
  mPulseCount(0),
  mWorkerThreads(),
  mWorkerContexts(),
  mManagerThread()
{
}


void ThreadPool::Start(const uint32_t count)
{
    THERON_ASSERT(count > 0);

    mThreadCount = 0;
    mTargetThreadCount = 0;

    // Set the target thread count, waking the manager thread and
    // causing it to create the initial set of worker threads.
    SetMinThreads(count);

    // Start the manager thread.
    mManagerThread.Start(StaticManagerThreadEntryPoint, this);

    // Wait until all worker threads have started completely before returning.
    // This guards against cases where the threads have already been told to
    // stop before they even get as far as checking the started flag, so they
    // terminate without ever checking the message queue.
    bool allThreadsStarted(false);
    while (!allThreadsStarted)
    {
        Lock lock(mManagerMonitor.GetMutex());
        allThreadsStarted = (mThreadCount >= mTargetThreadCount);
    }
}


void ThreadPool::Stop()
{
    // Set the target number of threads to zero.
    // On seeing this, the worker threads and manager thread terminate.
    // This call also wakes the manager thread so it quits, waiting for the workers to Join first.
    {
        Lock lock(mManagerMonitor.GetMutex());

        mTargetThreadCount = 0;
        mManagerMonitor.Pulse();
    }

    // Wake the worker threads so they terminate.
    {
        Lock lock(mWorkQueueMonitor.GetMutex());
        mWorkQueueMonitor.PulseAll();
    }

    // Wait for the manager thread to terminate.
    mManagerThread.Join();
}


void ThreadPool::SetMaxThreads(const uint32_t count)
{
    const uint32_t maxThreads(ClampThreadCount(count));
    Lock lock(mManagerMonitor.GetMutex());

    // Reduce the target thread count but don't increase it.
    if (mTargetThreadCount > maxThreads)
    {
        // Change the target value and wake the manager thread.
        // There's no point in waking the manager because it doesn't terminate the
        // threads - they terminate themselves the next time they awake.
        mTargetThreadCount = maxThreads;
    }
}


void ThreadPool::SetMinThreads(const uint32_t count)
{
    const uint32_t minThreads(ClampThreadCount(count));
    Lock lock(mManagerMonitor.GetMutex());

    // Increase the target thread count but don't reduce it.
    if (mTargetThreadCount < minThreads)
    {
        // Change the target value.
        mTargetThreadCount = minThreads;
        mManagerMonitor.Pulse();
    }
}


void ThreadPool::StaticWorkerThreadEntryPoint(void *const context)
{
    // The static entry point function is provided with a pointer to a context structure.
    // The context structure is specific to this worker thread.
    WorkerContext *const workerContext(reinterpret_cast<WorkerContext *>(context));

    // The thread entry point has to be a static function,
    // so in this static wrapper function we call the non-static method
    // on the instance, a pointer to which is provided in the context structure.
    workerContext->mThreadPool->WorkerThreadProc(workerContext);
}


void ThreadPool::WorkerThreadProc(WorkerContext *const workerContext)
{
    // This whole function is inside a lock-unlock pair. But the workers actually spend
    // most of their time outside the lock - either doing the processing of an item or
    // waiting for more work.
    Lock lock(mWorkQueueMonitor.GetMutex());

    while (true)
    {
        // Check the work queue for work.
        while (ActorCore *const actorCore = mWorkQueue.Pop())
        {
            // Process the actor and re-add it to the work queue if it needs more processing.
            ActorProcessor::Process(workerContext, mWorkQueue, lock, actorCore);
        }

        // We test this condition without locking the manager lock to reduce locking overheads.
        if (mThreadCount <= mTargetThreadCount)
        {
            // Wait for work to arrive or to be told to exit.
            // This releases the lock on the monitor and then re-acquires it when woken.
            mWorkQueueMonitor.Wait(lock);
            ++workerContext->mWakeCount;
        }
        else
        {
            // Terminate this thread if there are more threads than we want.
            Lock managerLock(mManagerMonitor.GetMutex());
            if (mThreadCount > mTargetThreadCount)
            {
                --mThreadCount;
                break;
            }
        }
    }

    // Mark the referenced context structure as no longer active.
    workerContext->mActive = false;
}


void ThreadPool::StaticManagerThreadEntryPoint(void *const context)
{
    ThreadPool *const threadPool(reinterpret_cast<ThreadPool *>(context));
    threadPool->ManagerThreadProc();
}


void ThreadPool::ManagerThreadProc()
{
    {
        Lock lock(mManagerMonitor.GetMutex());

        while (true)
        {
            // Start new threads while there are less than the target number.
            while (mThreadCount < mTargetThreadCount)
            {
                WorkerContext *workerContext(0);

                // Search for a previously created worker thread context which is no longer active.
                for (WorkerContextList::Iterator contexts(mWorkerContexts.Begin()); contexts != mWorkerContexts.End(); ++contexts)
                {
                    WorkerContext *const existingContext(*contexts);
                    if (!existingContext->mActive)
                    {
                        workerContext = existingContext;
                        break;
                    }
                }

                // If no existing worker context could be found, allocate a new one.
                if (workerContext == 0)
                {
                    // Allocate a worker thread context structure, cache-line aligned to discourage false-sharing.
                    // A pointer to the context is provided to the static thread function.
                    void *const contextMemory = AllocatorManager::Instance().GetAllocator()->AllocateAligned(sizeof(WorkerContext), THERON_CACHELINE_ALIGNMENT);
                    if (contextMemory == 0)
                    {
                        THERON_FAIL_MSG("Failed to allocate worker thread context structure");
                    }

                    // Construct the worker thread context structure, setting up a pointer to the owning threadpool.
                    // The context is marked active on construction, so that we don't reuse it prematurely.
                    workerContext = new (contextMemory) WorkerContext(this);

                    // Add the new context structure to the list.
                    mWorkerContexts.Insert(workerContext);
                }

                // Mark the worker thread context as active.
                workerContext->mActive = true;
                ++mThreadCount;

                lock.Unlock();

                // Create the worker thread.
                mWorkerThreads.CreateThread(StaticWorkerThreadEntryPoint, workerContext);

                lock.Relock();
            }

            // The manager terminates when the target thread count is set to zero.
            if (mTargetThreadCount == 0)
            {
                break;
            }

            // Go to sleep until we're woken again.
            // This releases the lock on the monitor and then re-acquires it when woken.
            mManagerMonitor.Wait(lock);
        }
    }

    // Wait for the worker threads to stop, and then delete them.
    mWorkerThreads.DestroyThreads();

    // Free the allocated worker context structures.
    while (!mWorkerContexts.Empty())
    {
        WorkerContext *const workerContext(mWorkerContexts.Front());
        mWorkerContexts.Remove(workerContext);

        // We expect all thread contexts to no longer be active.
        THERON_ASSERT(workerContext->mActive == false);

        // Call the worker context destructor explicitly since we constructed via in-place new, so can't use delete.
        workerContext->~WorkerContext();
        AllocatorManager::Instance().GetAllocator()->Free(workerContext, sizeof(WorkerContext));
    }
}


void ThreadPool::ResetCounters() const
{
    // Prevent the manager from messing with the thread context list while we read it.
    // Prevent the worker threads from trying to increment the counters at the same time.
    Lock workQueueLock(mWorkQueueMonitor.GetMutex());
    Lock managerLock(mManagerMonitor.GetMutex());

    // Also reset the per-framework pulse counter.
    mPulseCount = 0;

    for (WorkerContextList::Iterator contexts(mWorkerContexts.Begin()); contexts != mWorkerContexts.End(); ++contexts)
    {
        WorkerContext *const workerContext(*contexts);

        workerContext->mMessageCount = 0;
        workerContext->mPulseCount = 0;
        workerContext->mWakeCount = 0;
    }
}


uint32_t ThreadPool::GetNumMessagesProcessed() const
{
    uint32_t count(0);

    {
        // Prevent the manager from messing with the thread context list while we read it.
        Lock lock(mManagerMonitor.GetMutex());

        for (WorkerContextList::Iterator contexts(mWorkerContexts.Begin()); contexts != mWorkerContexts.End(); ++contexts)
        {
            const WorkerContext *const workerContext(*contexts);
            count += workerContext->mMessageCount;
        }
    }

    return count;
}


uint32_t ThreadPool::GetNumThreadsPulsed() const
{
    // The threadpool owns a per-framework counter which we need to include as well.
    // The per-framework counter is used to count pulsed caused by message sends in non-actor code.
    uint32_t count(mPulseCount);

    {
        // Prevent the manager from messing with the thread context list while we read it.
        Lock lock(mManagerMonitor.GetMutex());

        for (WorkerContextList::Iterator contexts(mWorkerContexts.Begin()); contexts != mWorkerContexts.End(); ++contexts)
        {
            const WorkerContext *const workerContext(*contexts);
            count += workerContext->mPulseCount;
        }
    }

    return count;
}


uint32_t ThreadPool::GetNumThreadsWoken() const
{
    uint32_t count(0);

    {
        // Prevent the manager from messing with the thread context list while we read it.
        Lock lock(mManagerMonitor.GetMutex());

        for (WorkerContextList::Iterator contexts(mWorkerContexts.Begin()); contexts != mWorkerContexts.End(); ++contexts)
        {
            const WorkerContext *const workerContext(*contexts);
            count += workerContext->mWakeCount;
        }
    }

    return count;
}


} // namespace Detail
} // namespace Theron


#ifdef _MSC_VER
#pragma warning(pop)
#endif //_MSC_VER

