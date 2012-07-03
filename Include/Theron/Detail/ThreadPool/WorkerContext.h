// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_DETAIL_THREADPOOL_WORKERCONTEXT_H
#define THERON_DETAIL_THREADPOOL_WORKERCONTEXT_H


#include <Theron/AllocatorManager.h>
#include <Theron/BasicTypes.h>
#include <Theron/Defines.h>

#include <Theron/Detail/Debug/Assert.h>
#include <Theron/Detail/Allocators/CachingAllocator.h>


namespace Theron
{
namespace Detail
{


class ThreadPool;


/**
Context structure holding per-thread data, passed to worker threads on creation.
*/
struct WorkerContext
{
    typedef CachingAllocator<32> MessageCacheType;

    THERON_FORCEINLINE explicit WorkerContext(ThreadPool * const threadPool) :
      mActive(true),
      mThreadPool(threadPool),
      mMessageCount(0),
      mPulseCount(0),
      mWakeCount(0),
      mMessageCache(AllocatorManager::Instance().GetAllocator()),
      mNext(0)
    {
        THERON_ASSERT(mThreadPool);
    }

    THERON_FORCEINLINE ~WorkerContext()
    {
        // Free any memory blocks left in the local message cache.
        mMessageCache.Clear();
    }

    /// Sets the pointer to the next context in a list of contexts.
    THERON_FORCEINLINE void SetNext(WorkerContext *const next)
    {
        mNext = next;
    }

    /// Gets the pointer to the next context in a list of contexts.
    THERON_FORCEINLINE WorkerContext *GetNext() const
    {
        return mNext;
    }

    bool mActive;                           ///< Flag marking context structures as in-use.
    ThreadPool *mThreadPool;                ///< A pointer to the owning thread pool.
    uint32_t mMessageCount;                 ///< Number of messages processed by this thread.
    uint32_t mPulseCount;                   ///< Number of times this thread pulsed the threadpool.
    uint32_t mWakeCount;                    ///< Number of times the thread has been woken.
    MessageCacheType mMessageCache;         ///< Cache of message memory blocks freed by this thread.
    WorkerContext *mNext;                   ///< Pointer to the next context in a list.
};


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_THREADPOOL_WORKERCONTEXT_H

