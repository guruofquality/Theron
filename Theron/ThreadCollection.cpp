// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#include <new>

#include <Theron/AllocatorManager.h>
#include <Theron/Assert.h>
#include <Theron/Defines.h>

#include <Theron/Detail/MailboxProcessor/ThreadCollection.h>
#include <Theron/Detail/Threading/Utils.h>


namespace Theron
{
namespace Detail
{


void ThreadCollection::StaticEntryPoint(void *const context)
{
    ThreadData *const threadData(reinterpret_cast<ThreadData *>(context));
    THERON_ASSERT(threadData);

    ThreadCollection *const threadCollection(threadData->mThreadCollection);
    Thread::EntryPoint userEntryPoint(threadData->mUserEntryPoint);
    void *const userContext(threadData->mUserContext);

    THERON_ASSERT(threadCollection);
    THERON_ASSERT(userEntryPoint);

    Utils::SetThreadAffinity(threadData->mNodeMask, threadData->mProcessorMask);

    // Call the wrapped user-level entry point function, passing it the user-level context data.
    userEntryPoint(userContext);

    // On return from the user-level entry point function, add the thread to the finished list.
    // This allows the calling thread (which is not this thread) to call Join() on it later.
    threadCollection->Finished(threadData);
}


ThreadCollection::ThreadCollection() :
  mSpinLock(),
  mThreads(),
  mFinishedThreads()
{
}


void ThreadCollection::CreateThread(
    Thread::EntryPoint userEntryPoint,
    void *const userContext,
    const uint32_t nodeMask,
    const uint32_t processorMask)
{
    THERON_ASSERT(userEntryPoint);

    Thread *thread(0);
    ThreadData *threadData(0);

    // Reuse one of the previously created threads that has finished, if available.
    mSpinLock.Lock();

    if (!mFinishedThreads.Empty())
    {
        threadData = mFinishedThreads.Front();
        mFinishedThreads.Remove(threadData);
    }

    mSpinLock.Unlock();

    if (threadData)
    {
        thread = threadData->mThread;

        // Join the finished thread and wait for it to terminate.
        // Join should be called from the same thread that called Start,
        // so we create and destroy all worker threads with the same manager thread.
        THERON_ASSERT(thread->Running());
        thread->Join();

        // Update the thread data with the new user entry point and user context.
        threadData->mNodeMask = nodeMask;
        threadData->mProcessorMask = processorMask;
        threadData->mUserEntryPoint = userEntryPoint;
        threadData->mUserContext = userContext;
    }
    else
    {
        // Allocate a new thread, aligning the memory to a cache-line boundary to reduce false-sharing of cache-lines.
        void *const threadMemory = AllocatorManager::Instance().GetAllocator()->AllocateAligned(sizeof(Thread), THERON_CACHELINE_ALIGNMENT);
        if (threadMemory == 0)
        {
            return;
        }

        // Allocate a thread data block.
        void *const dataMemory = AllocatorManager::Instance().GetAllocator()->Allocate(sizeof(ThreadData));
        if (dataMemory == 0)
        {
            AllocatorManager::Instance().GetAllocator()->Free(threadMemory, sizeof(Thread));
            return;
        }

        // Construct the thread and its data, setting up the data with a pointer to the thread and its context.
        // The thread pointer allows the thread function to be provided with a pointer to its thread object.
        thread = new (threadMemory) Thread();
        threadData = new (dataMemory) ThreadData(
            this,
            thread,
            nodeMask,
            processorMask,
            userEntryPoint,
            userContext);

        // Remember the threads we create in a list so we can eventually destroy them.
        mThreads.Insert(threadData);
    }

    // Start the thread, running it via our wrapper entry point.
    // We call our own wrapper entry point, passing it data including the user entry point and context.
    thread->Start(StaticEntryPoint, threadData);
}


void ThreadCollection::DestroyThreads()
{
    // Wait for the worker threads to stop.
    for (ThreadList::iterator it = mThreads.Begin(); it != mThreads.End(); ++it)
    {
        ThreadData *const threadData(*it);
        Thread *const thread(threadData->mThread);

        // Call Join on this thread; it shouldn't have been called yet.
        // This waits until the thread finishes.
        THERON_ASSERT(thread->Running());
        thread->Join();

        // Explicitly call the destructor, since we allocated using placement new.
        thread->~Thread();

        // Free the memory for the thread and the context block.
        AllocatorManager::Instance().GetAllocator()->Free(threadData, sizeof(ThreadData));
        AllocatorManager::Instance().GetAllocator()->Free(thread, sizeof(Thread));
    }

    mThreads.Clear();
    mFinishedThreads.Clear();
}


void ThreadCollection::Finished(ThreadData *const threadData)
{
    mSpinLock.Lock();
    mFinishedThreads.Insert(threadData);
    mSpinLock.Unlock();
}


} // namespace Detail
} // namespace Theron

