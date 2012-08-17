// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#include <Theron/Actor.h>
#include <Theron/AllocatorManager.h>
#include <Theron/Framework.h>

#include <Theron/Detail/Directory/StaticDirectory.h>
#include <Theron/Detail/Threading/Utils.h>


namespace Theron
{


Framework::Framework() :
  mIndex(0),
  mActorDirectory(),
  mMailboxes(),
  mWorkQueue(),
  mMailboxProcessor(mWorkQueue),
  mWorkerThreadStores(),
  mStoreAllocator(),
  mFallbackHandlers(),
  mDefaultFallbackHandler(),
  mMessageCache(AllocatorManager::Instance().GetAllocator()),
  mMessageAllocator(&mMessageCache),
  mProcessorContext(&mActorDirectory, &mMailboxes, &mWorkQueue, &mFallbackHandlers, &mMessageAllocator)
{
    const Parameters params;
    Initialize(params);
}


Framework::Framework(const uint32_t threadCount) :
  mIndex(0),
  mActorDirectory(),
  mMailboxes(),
  mWorkQueue(),
  mMailboxProcessor(mWorkQueue),
  mWorkerThreadStores(),
  mStoreAllocator(),
  mFallbackHandlers(),
  mDefaultFallbackHandler(),
  mMessageCache(AllocatorManager::Instance().GetAllocator()),
  mMessageAllocator(&mMessageCache),
  mProcessorContext(&mActorDirectory, &mMailboxes, &mWorkQueue, &mFallbackHandlers, &mMessageAllocator)
{
    const Parameters params(threadCount);
    Initialize(params);
}


Framework::Framework(const Parameters &params) :
  mIndex(0),
  mActorDirectory(),
  mMailboxes(),
  mWorkQueue(),
  mMailboxProcessor(mWorkQueue),
  mWorkerThreadStores(),
  mStoreAllocator(),
  mFallbackHandlers(),
  mDefaultFallbackHandler(),
  mMessageCache(AllocatorManager::Instance().GetAllocator()),
  mMessageAllocator(&mMessageCache),
  mProcessorContext(&mActorDirectory, &mMailboxes, &mWorkQueue, &mFallbackHandlers, &mMessageAllocator)
{
    Initialize(params);
}


Framework::~Framework()
{
    // Deregister the framework.
    Detail::StaticDirectory<Framework>::Deregister(mIndex);

    // Wait for the work queue to drain, to avoid memory leaks.
    uint32_t backoff(0);
    while (!mMailboxProcessor.Empty())
    {
        Detail::Utils::Backoff(backoff);
    }

    mMailboxProcessor.DestroyAllThreads();

    // Return any allocated worker thread storage structures to the context pool.
    while (!mWorkerThreadStores.Empty())
    {
        Detail::WorkerThreadStore *const context(mWorkerThreadStores.Front());
        mWorkerThreadStores.Remove(context);

        context->~WorkerThreadStore();
        mStoreAllocator.Free(context, sizeof(Detail::WorkerThreadStore));
    }
}


void Framework::Initialize(const Parameters &params)
{
    // Set up the default fallback handler, which catches and reports undelivered messages.
    SetFallbackHandler(&mDefaultFallbackHandler, &Detail::DefaultFallbackHandler::Handle);

    // Request the creation of the initial set of worker threads.
    uint32_t requestCount(0);
    for (uint32_t count(params.mThreadCount); count; --count)
    {
        // Use the pool to allocate a thread-local storage structure for the new worker thread.
        if (void *const storeMemory = mStoreAllocator.AllocateAligned(sizeof(Detail::WorkerThreadStore), THERON_CACHELINE_ALIGNMENT))
        {
            // In-place construct the storage structure in the allocated memory, passing in context info.
            // We pass in the per-framework processor context, from which the thread's context is initialized.
            Detail::WorkerThreadStore *const store = new (storeMemory) Detail::WorkerThreadStore(
                &mActorDirectory,
                &mMailboxes,
                &mWorkQueue,
                &mFallbackHandlers,
                &mMessageCache);

            // Remember the allocated worker thread store and create the worker thread.
            mWorkerThreadStores.Insert(store);

            // Create a worker thread with the given context on the given processor node.
            if (mMailboxProcessor.CreateThread(store, params.mNodeMask, params.mProcessorMask))
            {
                ++requestCount;
            }
        }
    }

    // Wait for all the worker threads to be started before returning.
    uint32_t threadCount(0);
    while (threadCount < requestCount)
    {
        threadCount = mMailboxProcessor.GetThreadCount();
    }

    // Register the framework and get a non-zero index for it, unique within the local process.
    mIndex = Detail::StaticDirectory<Framework>::Register(this);
    THERON_ASSERT(mIndex);
}


void Framework::RegisterActor(Actor *const actor)
{
    // Get the index of a free space in the directory.
    const uint32_t index(mActorDirectory.Allocate());

    // Register the actor.
    Detail::Entry &entry(mActorDirectory.GetEntry(index));

    entry.Lock();
    entry.SetEntity(actor);
    entry.Unlock();

    // Allocate the mailbox with the same index.
    mMailboxes.Allocate(index);

    // Create the unique address of the mailbox.
    const Address mailboxAddress(mIndex, index);

    // Set up the mailbox.
    Detail::Mailbox &mailbox(mMailboxes.GetEntry(index));
    mailbox.SetAddress(mailboxAddress);

    // Set the actor's entity address.
    // The address contains the index of the framework and the index of the actor within the framework.
    actor->mAddress = mailboxAddress;
}


void Framework::DeregisterActor(Actor *const actor)
{
    // Deregister the actor, so that the worker threads will leave it alone.
    const Address address(actor->GetAddress());
    const uint32_t index(address.AsInteger());
    Detail::Entry &entry(mActorDirectory.GetEntry(index));

    // If the entry is pinned then we have to wait for it to be unpinned.
    bool deregistered(false);
    while (!deregistered)
    {
        entry.Lock();

        if (!entry.IsPinned())
        {
            entry.Clear();
            deregistered = true;
        }

        entry.Unlock();
    }
}


} // namespace Theron
