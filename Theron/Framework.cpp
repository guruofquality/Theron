// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#include <new>

#include <Theron/Actor.h>
#include <Theron/Assert.h>
#include <Theron/AllocatorManager.h>
#include <Theron/Defines.h>
#include <Theron/EndPoint.h>
#include <Theron/Framework.h>
#include <Theron/IAllocator.h>

#include <Theron/Detail/Directory/StaticDirectory.h>
#include <Theron/Detail/Scheduler/BlockingScheduler.h>
#include <Theron/Detail/Scheduler/NonBlockingScheduler.h>
#include <Theron/Detail/Network/Index.h>
#include <Theron/Detail/Network/NameGenerator.h>
#include <Theron/Detail/Network/String.h>
#include <Theron/Detail/Threading/Utils.h>


#ifdef _MSC_VER
#pragma warning(push)
#pragma warning (disable:4996)  // function or variable may be unsafe
#endif //_MSC_VER


namespace Theron
{


void Framework::Initialize()
{
    // Create a shared scheduler context for the framework.
    // This scheduler context is used by worker threads when a per-thread context isn't available.
    mSharedWorkQueue = CreateWorkQueue();
    mSharedMailboxContext = CreateMailboxContext();
    mSharedScheduler = CreateSchedulerContext();

    mSharedScheduler->SetSharedWorkQueue(mSharedWorkQueue);
    mSharedScheduler->SetLocalWorkQueue(0);

    // The mailbox context holds a pointer to the scheduler's IScheduler interface.
    mSharedMailboxContext->mScheduler = mSharedScheduler;

    // Set the initial thread count and affinity masks.
    mThreadCount.Store(0);
    mTargetThreadCount.Store(mParams.mThreadCount);

    // Set up the default fallback handler, which catches and reports undelivered messages.
    SetFallbackHandler(&mDefaultFallbackHandler, &Detail::DefaultFallbackHandler::Handle);

    // Start the manager thread.
    mRunning = true;
    mManagerThread.Start(ManagerThreadEntryPoint, this);

    // Wait for the manager thread to start all the worker threads.
    uint32_t backoff(0);
    while (mThreadCount.Load() < mTargetThreadCount.Load())
    {
        Detail::Utils::Backoff(backoff);
    }

    // Register the framework and get a non-zero index for it, unique within the local process.
    mIndex = Detail::StaticDirectory<Framework>::Register(this);
    THERON_ASSERT(mIndex);

    // If the framework name wasn't set explicitly then generate a default name.
    if (mName.IsNull())
    {
        char buffer[16];
        Detail::NameGenerator::Generate(buffer, mIndex);
        mName = Detail::String(buffer);
    }
}


void Framework::Release()
{
    // Deregister the framework.
    Detail::StaticDirectory<Framework>::Deregister(mIndex);

    // Wait for the work queue to drain, to avoid memory leaks.
    uint32_t backoff(0);
    while (!QueuesEmpty())
    {
        Detail::Utils::Backoff(backoff);
    }

    // Reset the target thread count so the manager thread will kill all the threads.
    mTargetThreadCount.Store(0);

    // Wait for all the running threads to be stopped.
    backoff = 0;
    while (mThreadCount.Load() > 0)
    {
        // Pulse any threads that are waiting so they can terminate.
        mSharedWorkQueueCondition.PulseAll();

        Detail::Utils::Backoff(backoff);
    }

    // Kill the manager thread and wait for it to terminate.
    mRunning = false;
    mManagerThread.Join();

    // Destruct and free the per-framework scheduler context.
    DestroyWorkQueue(mSharedWorkQueue);
    mSharedWorkQueue = 0;

    DestroyMailboxContext(mSharedMailboxContext);
    mSharedMailboxContext = 0;

    DestroySchedulerContext(mSharedScheduler);
    mSharedScheduler = 0;
}


void Framework::RegisterActor(Actor *const actor, const char *const name)
{
    // Allocate an unused mailbox.
    const uint32_t mailboxIndex(mMailboxes.Allocate());
    Detail::Mailbox &mailbox(mMailboxes.GetEntry(mailboxIndex));

    // Use the provided name for the actor if one was provided.
    Detail::String mailboxName(name);
    if (name == 0)
    {
        char rawName[16];
        Detail::NameGenerator::Generate(rawName, mailboxIndex);

        const char *endPointName(0);
        if (mEndPoint)
        {
            endPointName = mEndPoint->GetName();        
        }

        char scopedName[256];
        Detail::NameGenerator::Combine(
            scopedName,
            256,
            rawName,
            mName.GetValue(),
            endPointName);

        mailboxName = Detail::String(scopedName);
    }

    // Name the mailbox and register the actor.
    mailbox.Lock();
    mailbox.SetName(mailboxName);
    mailbox.RegisterActor(actor);
    mailbox.Unlock();

    // Create the unique address of the mailbox.
    // Its a pair comprising the framework index and the mailbox index within the framework.
    const Detail::Index index(mIndex, mailboxIndex);
    const Address mailboxAddress(mailboxName, index);

    // Set the actor's mailbox address.
    // The address contains the index of the framework and the index of the mailbox within the framework.
    actor->mAddress = mailboxAddress;

    if (mEndPoint)
    {
        // Check that no mailbox with this name already exists.
        Detail::Index dummy;
        if (mEndPoint->Lookup(mailboxName, dummy))
        {
            THERON_FAIL_MSG("Can't create two actors or receivers with the same name");
        }
        
        // Register the mailbox with the endPoint so it can be found using its name.
        if (!mEndPoint->Register(mailboxName, index))
        {
            THERON_FAIL_MSG("Failed to register actor with the network endpoint");
        }
    }
}


void Framework::DeregisterActor(Actor *const actor)
{
    const Address address(actor->GetAddress());
    const Detail::String &mailboxName(address.GetName());

    // Deregister the mailbox with the endPoint so it can't be found anymore.
    if (mEndPoint)
    {
        mEndPoint->Deregister(mailboxName);
    }

    // Deregister the actor, so that the worker threads will leave it alone.
    const uint32_t mailboxIndex(address.AsInteger());
    Detail::Mailbox &mailbox(mMailboxes.GetEntry(mailboxIndex));

    // If the entry is pinned then we have to wait for it to be unpinned.
    bool deregistered(false);
    uint32_t backoff(0);

    while (!deregistered)
    {
        mailbox.Lock();

        if (!mailbox.IsPinned())
        {
            mailbox.DeregisterActor();
            deregistered = true;
        }

        mailbox.Unlock();

        Detail::Utils::Backoff(backoff);
    }
}


void Framework::SetMaxThreads(const uint32_t count)
{
    if (mTargetThreadCount.Load() > count)
    {
        mTargetThreadCount.Store(count);
    }
}


void Framework::SetMinThreads(const uint32_t count)
{
    if (mTargetThreadCount.Load() < count)
    {
        mTargetThreadCount.Store(count);
    }
}


uint32_t Framework::GetMaxThreads() const
{
    return mTargetThreadCount.Load();
}


uint32_t Framework::GetMinThreads() const
{
    return mTargetThreadCount.Load();
}


uint32_t Framework::GetNumThreads() const
{
    return mThreadCount.Load();
}


uint32_t Framework::GetPeakThreads() const
{
    return mPeakThreadCount.Load();
}


Detail::WorkQueue *Framework::CreateWorkQueue()
{
    IAllocator *const allocator(AllocatorManager::Instance().GetAllocator());

    void *const memory = allocator->AllocateAligned(
        sizeof(Detail::WorkQueue),
        THERON_CACHELINE_ALIGNMENT);

    if (memory == 0)
    {
        THERON_FAIL_MSG("Failed to allocate work queue");
    }

    Detail::WorkQueue *const workQueue = new (memory) Detail::WorkQueue();
    return workQueue;
}


void Framework::DestroyWorkQueue(Detail::WorkQueue *const workQueue)
{
    IAllocator *const allocator(AllocatorManager::Instance().GetAllocator());

    workQueue->~WorkQueue();
    allocator->Free(workQueue, sizeof(Detail::WorkQueue));
}


Detail::MailboxContext *Framework::CreateMailboxContext()
{
    IAllocator *const allocator(AllocatorManager::Instance().GetAllocator());

    void *const contextMemory = allocator->AllocateAligned(
        sizeof(Detail::MailboxContext),
        THERON_CACHELINE_ALIGNMENT);

    if (contextMemory == 0)
    {
        THERON_FAIL_MSG("Failed to allocate mailbox context");
    }

    Detail::MailboxContext *const mailboxContext = new (contextMemory)
        Detail::MailboxContext(&mMessageAllocator);

    mailboxContext->mMailboxes = &mMailboxes;
    mailboxContext->mFallbackHandlers = &mFallbackHandlers;

    return mailboxContext;
}


void Framework::DestroyMailboxContext(Detail::MailboxContext *const mailboxContext)
{
    IAllocator *const allocator(AllocatorManager::Instance().GetAllocator());

    mailboxContext->~MailboxContext();
    allocator->Free(mailboxContext, sizeof(Detail::MailboxContext));
}


Detail::IScheduler *Framework::CreateSchedulerContext()
{
    IAllocator *const allocator(AllocatorManager::Instance().GetAllocator());
    Detail::IScheduler *result(0);

    if (mParams.mYieldStrategy == YIELD_STRATEGY_BLOCKING)
    {
        void *const schedulerMemory = allocator->AllocateAligned(
            sizeof(Detail::BlockingScheduler),
            THERON_CACHELINE_ALIGNMENT);

        THERON_ASSERT_MSG(schedulerMemory, "Failed to allocate scheduler context");
        Detail::BlockingScheduler *const scheduler = new (schedulerMemory) Detail::BlockingScheduler();

        // Set up the aspects of the scheduler that are specific to this implementation.
        scheduler->SetSharedWorkQueueCondition(&mSharedWorkQueueCondition);

        result = scheduler;
    }
    else
    {
        void *const schedulerMemory = allocator->AllocateAligned(
            sizeof(Detail::NonBlockingScheduler),
            THERON_CACHELINE_ALIGNMENT);

        THERON_ASSERT_MSG(schedulerMemory, "Failed to allocate scheduler context");
        Detail::NonBlockingScheduler *const scheduler = new (schedulerMemory) Detail::NonBlockingScheduler();

        // Set up the aspects of the scheduler that are specific to this implementation.
        scheduler->SetSharedWorkQueueSpinLock(&mSharedWorkQueueSpinLock);
        scheduler->SetYieldStrategy(mParams.mYieldStrategy);

        result = scheduler;
    }

    return result;
}


void Framework::DestroySchedulerContext(Detail::IScheduler *const scheduler)
{
    IAllocator *const allocator(AllocatorManager::Instance().GetAllocator());

    scheduler->~IScheduler();
    allocator->Free(scheduler);
}


bool Framework::QueuesEmpty() const
{
    bool empty(true);

    mSharedWorkQueueSpinLock.Lock();

    if (!mSharedWorkQueue->Empty())
    {
        empty = false;
    }

    mSharedWorkQueueSpinLock.Unlock();

    if (!empty)
    {
        return false;
    }

    mThreadContextLock.Lock();
    
    // Check the queues in all worker contexts.
    ContextList::Iterator contexts(mThreadContexts.GetIterator());
    while (contexts.Next())
    {
        Detail::ThreadPool::ThreadContext *const threadContext(contexts.Get());

        if (!threadContext->mScheduler->GetLocalWorkQueue()->Empty())
        {
            empty = false;
            break;
        }
    }

    mThreadContextLock.Unlock();

    return empty;
}


void Framework::ResetCounters() const
{
    mThreadContextLock.Lock();

    // Reset the counters in all thread contexts.
    ContextList::Iterator contexts(mThreadContexts.GetIterator());
    while (contexts.Next())
    {
        Detail::ThreadPool::ThreadContext *const threadContext(contexts.Get());

        for (uint32_t index = 0; index < (uint32_t) MAX_COUNTERS; ++index)
        {
            threadContext->mMailboxContext->mCounters[index].Store(0);
        }
    }

    mThreadContextLock.Unlock();
}


uint32_t Framework::GetCounterValue(const Counter counter) const
{
    uint32_t total(0);

    mThreadContextLock.Lock();

    // Accumulate the counter values from all thread contexts.
    ContextList::Iterator contexts(mThreadContexts.GetIterator());
    while (contexts.Next())
    {
        Detail::ThreadPool::ThreadContext *const threadContext(contexts.Get());

        total += threadContext->mMailboxContext->mCounters[counter].Load();
    }

    mThreadContextLock.Unlock();

    return total;
}


uint32_t Framework::GetPerThreadCounterValues(
    const Counter counter,
    uint32_t *const perThreadCounts,
    const uint32_t maxCounts) const
{
    uint32_t itemCount(0);

    mThreadContextLock.Lock();

    // Read the per-thread counter values into the provided array, skipping non-running contexts.
    ContextList::Iterator contexts(mThreadContexts.GetIterator());
    while (itemCount < maxCounts && contexts.Next())
    {
        Detail::ThreadPool::ThreadContext *const threadContext(contexts.Get());

        if (Detail::ThreadPool::IsRunning(threadContext))
        {
            perThreadCounts[itemCount++] = threadContext->mMailboxContext->mCounters[counter].Load();
        }
    }

    mThreadContextLock.Unlock();

    return itemCount;
}


void Framework::ManagerThreadEntryPoint(void *const context)
{
    // The static entry point function is passed the object pointer as context.
    Framework *const framework(reinterpret_cast<Framework *>(context));
    framework->ManagerThreadProc();
}


void Framework::ManagerThreadProc()
{
    IAllocator *const allocator(AllocatorManager::Instance().GetAllocator());

    while (mRunning)
    {
        mThreadContextLock.Lock();

        // Re-start stopped worker threads while the thread count is too low.
        ContextList::Iterator contexts(mThreadContexts.GetIterator());
        while (mThreadCount.Load() < mTargetThreadCount.Load() && contexts.Next())
        {
            Detail::ThreadPool::ThreadContext *const threadContext(contexts.Get());

            if (!Detail::ThreadPool::IsRunning(threadContext))
            {
                if (!Detail::ThreadPool::StartThread(
                    threadContext,
                    mParams.mNodeMask,
                    mParams.mProcessorMask))
                {
                    break;
                }

                mThreadCount.Increment();
            }
        }

        // Create new worker threads while the thread count is still too low.
        while (mThreadCount.Load() < mTargetThreadCount.Load())
        {
            Detail::WorkQueue *const workQueue(CreateWorkQueue());
            Detail::MailboxContext *const mailboxContext(CreateMailboxContext());
            Detail::IScheduler *const scheduler(CreateSchedulerContext());

            // Set up the work queue pointers in this per-thread context.
            // The per-thread contexts have pointers to the single shared queue and their own owned queues.
            scheduler->SetSharedWorkQueue(mSharedWorkQueue);
            scheduler->SetLocalWorkQueue(workQueue);

            // The mailbox context holds a pointer to the scheduler's IScheduler interface.
            mailboxContext->mScheduler = scheduler;

            // Create a thread context structure wrapping the worker context.
            void *const contextMemory = allocator->AllocateAligned(sizeof(Detail::ThreadPool::ThreadContext), THERON_CACHELINE_ALIGNMENT);
            if (contextMemory == 0)
            {
                DestroyWorkQueue(workQueue);
                DestroyMailboxContext(mailboxContext);
                DestroySchedulerContext(scheduler);
                break;
            }

            Detail::ThreadPool::ThreadContext *const threadContext = new (contextMemory)
                Detail::ThreadPool::ThreadContext(mailboxContext, scheduler);

            // Create a worker thread with the created context.
            if (!Detail::ThreadPool::CreateThread(threadContext))
            {
                DestroyWorkQueue(workQueue);
                DestroyMailboxContext(mailboxContext);
                DestroySchedulerContext(scheduler);
                allocator->Free(threadContext);
                break;
            }

            // Start the thread on the given node and processors.
            if (!Detail::ThreadPool::StartThread(
                threadContext,
                mParams.mNodeMask,
                mParams.mProcessorMask))
            {
                DestroyWorkQueue(workQueue);
                DestroyMailboxContext(mailboxContext);
                DestroySchedulerContext(scheduler);
                allocator->Free(threadContext);
                break;
            }

            // Remember the context so we can reuse it and eventually destroy it.
            mThreadContexts.Insert(threadContext);

            // Track the peak thread count.
            mThreadCount.Increment();
            if (mThreadCount.Load() > mPeakThreadCount.Load())
            {
                mPeakThreadCount.Store(mThreadCount.Load());
            }
        }

        // Stop some running worker threads while the thread count is too high.
        contexts = mThreadContexts.GetIterator();
        while (mThreadCount.Load() > mTargetThreadCount.Load() && contexts.Next())
        {
            Detail::ThreadPool::ThreadContext *const threadContext(contexts.Get());
            if (Detail::ThreadPool::IsRunning(threadContext))
            {
                // Mark the thread as stopped and wake all the waiting threads.
                Detail::ThreadPool::StopThread(threadContext);
                mSharedWorkQueueCondition.PulseAll();

                // Wait for the stopped thread to actually terminate.
                Detail::ThreadPool::JoinThread(threadContext);

                mThreadCount.Decrement();
            }
        }

        mThreadContextLock.Unlock();

        // The manager thread spends most of its time asleep.
        Detail::Utils::SleepThread(10);
    }

    // Free all the allocated thread context objects.
    while (!mThreadContexts.Empty())
    {
        Detail::ThreadPool::ThreadContext *const threadContext(mThreadContexts.Front());
        mThreadContexts.Remove(threadContext);

        // Wait for the thread to stop and then destroy it.
        Detail::ThreadPool::DestroyThread(threadContext);

        // Destruct and free the per-worker-thread storage.
        DestroyWorkQueue(threadContext->mScheduler->GetLocalWorkQueue());
        DestroyMailboxContext(threadContext->mMailboxContext);
        DestroySchedulerContext(threadContext->mScheduler);

        // Destruct and free the per-thread context.
        threadContext->~ThreadContext();
        allocator->Free(threadContext, sizeof(Detail::ThreadPool::ThreadContext));
    }
}


} // namespace Theron


#ifdef _MSC_VER
#pragma warning(pop)
#endif //_MSC_VER
