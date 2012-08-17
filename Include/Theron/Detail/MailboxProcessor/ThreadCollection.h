// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_DETAIL_MAILBOXPROCESSOR_THREADCOLLECTION_H
#define THERON_DETAIL_MAILBOXPROCESSOR_THREADCOLLECTION_H


#include <Theron/BasicTypes.h>
#include <Theron/Defines.h>

#include <Theron/Detail/Containers/List.h>
#include <Theron/Detail/Threading/Thread.h>
#include <Theron/Detail/Threading/SpinLock.h>


namespace Theron
{
namespace Detail
{


/**
A collection of threads.
*/
class ThreadCollection
{
public:

    /**
    Entry point wrapper function run by each started thread.
    */
    static void StaticEntryPoint(void *const context);

    /**
    Default constructor.
    */
    ThreadCollection();

    /**
    Creates a new worker thread.
    */
    void CreateThread(
        Thread::EntryPoint userEntryPoint,
        void *const userContext,
        const uint32_t nodeMask,
        const uint32_t processorMask);

    /**
    Destroys all previously created worker threads.
    */
    void DestroyThreads();

    /**
    Returns the number of threads currently in the collection.
    \note Some of the threads may may be dormant and no longer running.
    */
    inline uint32_t Size() const;

private:

    /**
    Record that serves as a context scratch pad for a created thread.
    */
    struct ThreadData
    {
        THERON_FORCEINLINE ThreadData(
            ThreadCollection * const threadCollection,
            Thread * const thread,
            const uint32_t nodeMask,
            const uint32_t processorMask,
            Thread::EntryPoint entryPoint,
            void *const userContext) :
          mThreadCollection(threadCollection),
          mThread(thread),
          mNodeMask(nodeMask),
          mProcessorMask(processorMask),
          mUserEntryPoint(entryPoint),
          mUserContext(userContext)
        {
            THERON_ASSERT(mThreadCollection);
            THERON_ASSERT(mThread);
            THERON_ASSERT(mUserEntryPoint);
        }

        ThreadCollection *mThreadCollection;        ///< Used to notify the collection when the thread finishes.
        Thread *mThread;                            ///< Pointer to the thread object.
        uint32_t mNodeMask;                         ///< Bitfield NUMA node affinity mask for the created thread.
        uint32_t mProcessorMask;                    ///< Bitfield processor affinity mask for the created thread.
        Thread::EntryPoint mUserEntryPoint;         ///< The entry point function to be executed by the thread.
        void *mUserContext;                         ///< The data to be passed to the entry point function.
    };

    typedef List<ThreadData *> ThreadList;

    ThreadCollection(const ThreadCollection &other);
    ThreadCollection &operator=(const ThreadCollection &other);

    /**
    Marks the given thread as finished, making it available for reuse.
    */
    void Finished(ThreadData *const threadData);

    SpinLock mSpinLock;                 ///< Ensures threadsafe access to owned lists.
    ThreadList mThreads;                ///< All the thread objects we've created.
    ThreadList mFinishedThreads;        ///< Threads which have terminated and need joining.
};


THERON_FORCEINLINE uint32_t ThreadCollection::Size() const
{
    return mThreads.Size();
}


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_MAILBOXPROCESSOR_THREADCOLLECTION_H
