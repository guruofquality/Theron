// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#include <new>

#include <Theron/Detail/Debug/Assert.h>
#include <Theron/Detail/Threading/Lock.h>
#include <Theron/Detail/ThreadPool/ThreadCollection.h>

#include <Theron/AllocatorManager.h>
#include <Theron/Defines.h>


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

    // Call the wrapped user-level entry point function, passing it the user-level context data.
    userEntryPoint(userContext);

    // On return from the user-level entry point function, add the thread to the finished list.
    // This allows the calling thread (which is not this thread) to call Join() on it later.
    threadCollection->Finished(threadData);
}


ThreadCollection::ThreadCollection() :
  mMutex(),
  mThreads(),
  mFinishedThreads()
{
}


void ThreadCollection::CreateThread(Thread::EntryPoint userEntryPoint, void *const userContext)
{
    THERON_ASSERT(userEntryPoint);

    Thread *thread(0);
    ThreadData *threadData(0);

    // Reuse one of the previously created threads that has finished, if available.
    {
        Lock lock(mMutex);

        if (!mFinishedThreads.Empty())
        {
            threadData = mFinishedThreads.Front();
            mFinishedThreads.Remove(threadData);
        }
    }

    if (threadData)
    {
        thread = threadData->mThread;

        // Join the finished thread and wait for it to terminate.
        // Join should be called from the same thread that called Start,
        // so we create and destroy all worker threads with the same manager thread.
        THERON_ASSERT(thread->Running());
        thread->Join();

        // Update the thread data with the new user entry point and user context.
        threadData->mUserEntryPoint = userEntryPoint;
        threadData->mUserContext = userContext;
    }
    else
    {
        // Allocate a new thread.
        void *const threadMemory = AllocatorManager::Instance().GetAllocator()->Allocate(sizeof(Thread));
        if (threadMemory == 0)
        {
            return;
        }

        // Allocate a thread data block.
        void *const dataMemory = AllocatorManager::Instance().GetAllocator()->Allocate(sizeof(ThreadData));
        if (dataMemory == 0)
        {
            AllocatorManager::Instance().GetAllocator()->Free(threadMemory);
            return;
        }

        // Construct the thread and its data, setting up the data with a pointer to the thread and its context.
        // The thread pointer allows the thread function to be provided with a pointer to its thread object.
        thread = new (threadMemory) Thread();
        threadData = new (dataMemory) ThreadData(
            this,
            thread,
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
        AllocatorManager::Instance().GetAllocator()->Free(threadData);
        AllocatorManager::Instance().GetAllocator()->Free(thread);
    }

    mThreads.Clear();
    mFinishedThreads.Clear();
}


void ThreadCollection::Finished(ThreadData *const threadData)
{
    Lock lock(mMutex);
    mFinishedThreads.Insert(threadData);
}


} // namespace Detail
} // namespace Theron

