// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_DETAIL_THREADING_STD_THREAD_H
#define THERON_DETAIL_THREADING_STD_THREAD_H


#ifdef _MSC_VER
#pragma warning(push,0)
#endif //_MSC_VER

#include <new>
#include <thread>

#ifdef _MSC_VER
#pragma warning(pop)
#endif //_MSC_VER

#include <Theron/Detail/Debug/Assert.h>
#include <Theron/AllocatorManager.h>
#include <Theron/Defines.h>


namespace Theron
{
namespace Detail
{


/// Thread class implementing a simple system thread.
/// \note This implementation uses C++0x threads so requires C++0x compiler (e.g. GCC >= 4.6
class Thread
{
public:

    /// Defines a function that can serve as a thread entry point.
    /// \note Entry point functions must be static -- implying that they can't be
    /// non-static class member functions.
    typedef void (*EntryPoint)(void *const context);

    /// Default constructor
    THERON_FORCEINLINE Thread() : mThread(0)
    {
    }

    /// Destructor
    THERON_FORCEINLINE ~Thread()
    {
        THERON_ASSERT(mThread == 0);
    }

    /// Starts the thread, executing the given entry point function.
    /// \param entryPoint The entry point function that the thread should execute.
    /// \param context Pointer to a context object providing the environment in which the thread runs.
    /// \return True, if the thread was started successfully.
    inline bool Start(EntryPoint entryPoint, void *const context);

    /// Waits for the thread to finish and return.
    /// The semantics are that Start and Join can be called repeatedly in pairs.
    inline void Join();

    /// Returns true if the thread is currently running.
    /// The thread is running if Start was called more recently than Join.
    inline bool Running() const;

private:

    /// A callable object implementing operator() that starts a thread.
    /// Basically a struct that holds a pointer to a thread entry point function and some context data.
    class ThreadStarter
    {
    public:
    
        THERON_FORCEINLINE ThreadStarter(EntryPoint entryPoint, void *const context) :
          mEntryPoint(entryPoint),
          mContext(context)
        {
        }

        THERON_FORCEINLINE void operator()()
        {
            mEntryPoint(mContext);
        }

    private:
    
        EntryPoint mEntryPoint;
        void *mContext;
    };

    Thread(const Thread &other);
    Thread &operator=(const Thread &other);

    std::thread *mThread;     ///< Pointer to the owned std::thread.
};


THERON_FORCEINLINE bool Thread::Start(EntryPoint entryPoint, void *const context)
{
    THERON_ASSERT(mThread == 0);

    // Allocate memory for a std::thread object. They're not copyable.
    void *const memory = AllocatorManager::Instance().GetAllocator()->Allocate(sizeof(std::thread));
    if (memory == 0)
    {
        return false;
    }

    // Construct a std::thread in the allocated memory
    // Pass it a callable object that in turn calls the entry point, passing it some context.
    ThreadStarter starter(entryPoint, context);
    mThread = new (memory) std::thread(starter);

    if (mThread == 0)
    {
        return false;
    }

    return true;
}


THERON_FORCEINLINE void Thread::Join()
{
    THERON_ASSERT(mThread);

    // This waits for the thread function to return.
    mThread->join();

    // We destroy the underlying thread object and recreate it next time.
    // Explicitly destruct because allocated with placement new.
    mThread->~thread();

    AllocatorManager::Instance().GetAllocator()->Free(mThread, sizeof(std::thread)); 
    mThread = 0;
}


THERON_FORCEINLINE bool Thread::Running() const
{
    return (mThread != 0);
}


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_THREADING_STD_THREAD_H

