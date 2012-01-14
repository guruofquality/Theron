// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#include <Theron/Detail/ThreadPool/ThreadPool.h>


#ifdef _MSC_VER
#pragma warning(push)
#pragma warning (disable:4127)  // Conditional expression is constant.
#endif //_MSC_VER


namespace Theron
{
namespace Detail
{


ThreadPool::ThreadPool() :
  mNumThreads(0),
  mTargetThreads(0),
  mWorkQueue(),
  mWorkQueueMonitor(),
  mManagerMonitor(),
  mNumMessagesProcessed(0),
  mNumThreadsPulsed(0),
  mNumThreadsWoken(0),
  mWorkerThreads(),
  mManagerThread()
{
}


void ThreadPool::Start(const uint32_t count)
{
    THERON_ASSERT(count > 0);

    mNumThreads = 0;
    mTargetThreads = 0;

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
        allThreadsStarted = (mNumThreads >= mTargetThreads);
    }
}


void ThreadPool::Stop()
{
    // Set the target number of threads to zero.
    // On seeing this, the worker threads and manager thread terminate.
    // This call also wakes the manager thread so it quits, waiting for the workers to Join first.
    {
        Lock lock(mManagerMonitor.GetMutex());

        mTargetThreads = 0;
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
    if (mTargetThreads > maxThreads)
    {
        // Change the target value and wake the manager thread.
        // There's no point in waking the manager because it doesn't terminate the
        // threads - they terminate themselves the next time they awake.
        mTargetThreads = maxThreads;
    }
}


void ThreadPool::SetMinThreads(const uint32_t count)
{
    const uint32_t minThreads(ClampThreadCount(count));
    Lock lock(mManagerMonitor.GetMutex());

    // Increase the target thread count but don't reduce it.
    if (mTargetThreads < minThreads)
    {
        // Change the target value.
        mTargetThreads = minThreads;
        mManagerMonitor.Pulse();
    }
}


void ThreadPool::StaticWorkerThreadEntryPoint(void *const context)
{
    // The thread entry point has to be a static function,
    // so in this static wrapper function we call the non-static method
    // on the instance, which is provided as context.

    ThreadPool *const threadPool(reinterpret_cast<ThreadPool *>(context));
    threadPool->WorkerThreadProc();
}


void ThreadPool::WorkerThreadProc()
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
            ProcessActorCore(lock, actorCore);
        }

        // We test this condition without locking the manager lock to reduce locking overheads.
        if (mNumThreads <= mTargetThreads)
        {
            // Wait for work to arrive or to be told to exit.
            // This releases the lock on the monitor and then re-acquires it when woken.
            mWorkQueueMonitor.Wait(lock);
            ++mNumThreadsWoken;
        }
        else
        {
            // Terminate this thread if there are more threads than we want.
            Lock managerLock(mManagerMonitor.GetMutex());
            if (mNumThreads > mTargetThreads)
            {
                --mNumThreads;
                break;
            }
        }
    }
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
            while (mNumThreads < mTargetThreads)
            {
                lock.Unlock();

                mWorkerThreads.CreateThread(StaticWorkerThreadEntryPoint, this);

                lock.Relock();
                ++mNumThreads;
            }

            // The manager terminates when the target thread count is set to zero.
            if (mTargetThreads > 0)
            {
                // Go to sleep and until we're woken again.
                // This releases the lock on the monitor and then re-acquires it when woken.
                mManagerMonitor.Wait(lock);
            }
            else
            {
                break;
            }
        }
    }

    // Wait for the worker threads to stop, and then delete them.
    mWorkerThreads.DestroyThreads();
}


} // namespace Detail
} // namespace Theron


#ifdef _MSC_VER
#pragma warning(pop)
#endif //_MSC_VER

