// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#include <new>

#include <Theron/Actor.h>
#include <Theron/Assert.h>
#include <Theron/AllocatorManager.h>
#include <Theron/EndPoint.h>
#include <Theron/Framework.h>
#include <Theron/IAllocator.h>

#include <Theron/Detail/Directory/StaticDirectory.h>
#include <Theron/Detail/Network/Index.h>
#include <Theron/Detail/Network/NameGenerator.h>
#include <Theron/Detail/Threading/Utils.h>


namespace Theron
{


Framework::Framework(const uint32_t threadCount) :
  mEndPoint(0),
  mParams(threadCount),
  mIndex(0),
  mName(),
  mMailboxes(),
  mWorkQueue(),
  mFallbackHandlers(),
  mDefaultFallbackHandler(),
  mMessageCache(AllocatorManager::Instance().GetAllocator()),
  mMessageAllocator(&mMessageCache),
  mProcessorContext(&mMailboxes, &mWorkQueue, &mFallbackHandlers, &mMessageAllocator),
  mManagerThread(),
  mRunning(false),
  mTargetThreadCount(0),
  mPeakThreadCount(0),
  mThreadCount(0),
  mThreadContexts(),
  mThreadContextLock(),
  mNodeMask(0),
  mProcessorMask(0)
{
    Initialize();
}


Framework::Framework(const Parameters &params) :
  mEndPoint(0),
  mParams(params),
  mIndex(0),
  mName(),
  mMailboxes(),
  mWorkQueue(),
  mFallbackHandlers(),
  mDefaultFallbackHandler(),
  mMessageCache(AllocatorManager::Instance().GetAllocator()),
  mMessageAllocator(&mMessageCache),
  mProcessorContext(&mMailboxes, &mWorkQueue, &mFallbackHandlers, &mMessageAllocator),
  mManagerThread(),
  mRunning(false),
  mTargetThreadCount(0),
  mPeakThreadCount(0),
  mThreadCount(0),
  mThreadContexts(),
  mThreadContextLock(),
  mNodeMask(0),
  mProcessorMask(0)
{
    Initialize();
}


Framework::Framework(EndPoint &endPoint, const char *const name, const Parameters &params) :
  mEndPoint(&endPoint),
  mParams(params),
  mIndex(0),
  mName(name),
  mMailboxes(),
  mWorkQueue(),
  mFallbackHandlers(),
  mDefaultFallbackHandler(),
  mMessageCache(AllocatorManager::Instance().GetAllocator()),
  mMessageAllocator(&mMessageCache),
  mProcessorContext(&mMailboxes, &mWorkQueue, &mFallbackHandlers, &mMessageAllocator),
  mManagerThread(),
  mRunning(false),
  mTargetThreadCount(0),
  mPeakThreadCount(0),
  mThreadCount(0),
  mThreadContexts(),
  mThreadContextLock(),
  mNodeMask(0),
  mProcessorMask(0)
{
    Initialize();
}


Framework::~Framework()
{
    Release();
}


void Framework::Initialize()
{
    // Set the initial thread count and affinity masks.
    mThreadCount.Store(0);
    mTargetThreadCount.Store(mParams.mThreadCount);

    mNodeMask = mParams.mNodeMask;
    mProcessorMask = mParams.mProcessorMask;

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
        mName = Detail::NameGenerator::Generate(mIndex);
    }
}


void Framework::Release()
{
    // Deregister the framework.
    Detail::StaticDirectory<Framework>::Deregister(mIndex);

    // Wait for the work queue to drain, to avoid memory leaks.
    uint32_t backoff(0);
    while (!mWorkQueue.Empty())
    {
        Detail::Utils::Backoff(backoff);
    }

    // Reset the target thread count so the manager thread will kill all the threads.
    mTargetThreadCount.Store(0);

    // Wait for all the running threads to be stopped.
    backoff = 0;
    while (mThreadCount.Load() > 0)
    {
        Detail::Utils::Backoff(backoff);
    }

    // Kill the manager thread and wait for it to terminate.
    mRunning = false;
    mManagerThread.Join();
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
        // Generate a default string name for the mailbox.
        // The default name is a composite of the mailbox index, framework index, and endpoint name.
        Detail::String rawName(Detail::NameGenerator::Generate(mailboxIndex));

        const char *endPointName(0);
        if (mEndPoint)
        {
            endPointName = mEndPoint->GetName();        
        }

        mailboxName = Detail::NameGenerator::Combine(
            rawName.GetValue(),
            mName.GetValue(),
            endPointName);
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


void Framework::ResetCounters() const
{
    mThreadContextLock.Lock();

    // Reset the counters in all thread contexts.
    ContextList::Iterator contexts(mThreadContexts.GetIterator());
    while (contexts.Next())
    {
        ThreadPool::ThreadContext *const threadContext(contexts.Get());

        for (uint32_t index = 0; index < (uint32_t) MAX_COUNTERS; ++index)
        {
            threadContext->mWorkerContext->mProcessorContext.mCounters[index].Store(0);
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
        ThreadPool::ThreadContext *const threadContext(contexts.Get());

        total += threadContext->mWorkerContext->mProcessorContext.mCounters[counter].Load();
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
        ThreadPool::ThreadContext *const threadContext(contexts.Get());

        if (ThreadPool::IsRunning(threadContext))
        {
            perThreadCounts[itemCount++] = threadContext->mWorkerContext->mProcessorContext.mCounters[counter].Load();
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
            ThreadPool::ThreadContext *const threadContext(contexts.Get());

            if (!ThreadPool::IsRunning(threadContext))
            {
                if (!ThreadPool::StartThread(
                    threadContext,
                    &mWorkQueue,
                    mNodeMask,
                    mProcessorMask))
                {
                    break;
                }

                mThreadCount.Increment();
            }
        }

        // Create new worker threads while the thread count is still too low.
        while (mThreadCount.Load() < mTargetThreadCount.Load())
        {
            // Create a worker context for this thread.
            void *const storeMemory = allocator->AllocateAligned(sizeof(Detail::WorkerThreadStore), THERON_CACHELINE_ALIGNMENT);
            if (storeMemory == 0)
            {
                continue;
            }

            Detail::WorkerThreadStore *const store = new (storeMemory) Detail::WorkerThreadStore(
                &mMailboxes,
                &mWorkQueue,
                &mFallbackHandlers,
                &mMessageCache);

            // Create a thread context structure wrapping the worker context.
            void *const contextMemory = allocator->AllocateAligned(sizeof(ThreadPool::ThreadContext), THERON_CACHELINE_ALIGNMENT);
            if (contextMemory == 0)
            {
                allocator->Free(storeMemory);
                break;
            }

            ThreadPool::ThreadContext *const threadContext = new (contextMemory) ThreadPool::ThreadContext(store);

            // Create a worker thread with the created context.
            if (!ThreadPool::CreateThread(threadContext))
            {
                allocator->Free(storeMemory);
                allocator->Free(threadContext);
                break;
            }

            // Start the thread on the given node and processors.
            if (!ThreadPool::StartThread(
                threadContext,
                &mWorkQueue,
                mNodeMask,
                mProcessorMask))
            {
                allocator->Free(storeMemory);
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
            ThreadPool::ThreadContext *const threadContext(contexts.Get());
            if (ThreadPool::IsRunning(threadContext))
            {
                ThreadPool::StopThread(threadContext);
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
        ThreadPool::ThreadContext *const threadContext(mThreadContexts.Front());
        mThreadContexts.Remove(threadContext);

        // Wait for the thread to stop and then destroy it.
        ThreadPool::DestroyThread(threadContext);

        // Destruct and free the per-worker-thread storage.
        threadContext->mWorkerContext->~WorkerThreadStore();
        allocator->Free(threadContext->mWorkerContext, sizeof(Detail::WorkerThreadStore));

        // Destruct and free the per-thread context.
        threadContext->~ThreadContext();
        allocator->Free(threadContext, sizeof(ThreadPool::ThreadContext));
    }
}


} // namespace Theron
