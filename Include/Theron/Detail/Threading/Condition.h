// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_DETAIL_THREADING_CONDITION_H
#define THERON_DETAIL_THREADING_CONDITION_H


#include <Theron/Assert.h>
#include <Theron/BasicTypes.h>
#include <Theron/Defines.h>

#include <Theron/Detail/Threading/Lock.h>
#include <Theron/Detail/Threading/Mutex.h>


#ifdef _MSC_VER
#pragma warning(push,0)
#endif // _MSC_VER

#if THERON_WINDOWS

#include <windows.h>

#elif THERON_BOOST

#include <boost/thread/condition_variable.hpp>
#include <boost/thread/locks.hpp>

#elif THERON_CPP11

#error CPP11 support not implemented yet.

#elif defined(THERON_POSIX)

#error POSIX support not implemented yet.

#else

#error No condition variable support detected.

#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER


namespace Theron
{
namespace Detail
{


/**
Portable condition variable synchronization primitive, sometimes also called a monitor.
*/
class Condition
{
public:

#if THERON_WINDOWS

    enum
    {
        PULSE_EVENT = 0,        ///< Event used to wake a single waiting thread.
        PULSE_ALL_EVENT = 1     ///< Event used to wake all waiting threads.
    };

#endif

    /**
    Constructor.
    */
    inline Condition()
    {
#if THERON_WINDOWS

        // This event wakes a single waiting thread.
        mEvents[PULSE_EVENT] = CreateEvent(
            0,                                  // No security attributes
            false,                              // Manual reset disabled
            false,                              // Initial state: reset
            0);                                 // event name (none)

        // This event wakes all the waiting threads
        mEvents[PULSE_ALL_EVENT] = CreateEvent(
            0,                                  // No security attributes
            true,                               // Manual reset enabled
            false,                              // Initial state: reset
            0);                                 // event name (none)

        THERON_ASSERT(mEvents[PULSE_EVENT]);
        THERON_ASSERT(mEvents[PULSE_ALL_EVENT]);

#elif THERON_BOOST
#elif THERON_CPP11
#elif defined(THERON_POSIX)
#endif
    }

    /**
    Destructor.
    \note If the condition is destroyed while threads are still waiting on it, the result is undefined.
    */
    inline ~Condition()
    {
#if THERON_WINDOWS

        THERON_ASSERT(mEvents[PULSE_EVENT]);
        THERON_ASSERT(mEvents[PULSE_ALL_EVENT]);

        CloseHandle(mEvents[PULSE_EVENT]);
        CloseHandle(mEvents[PULSE_ALL_EVENT]);

#elif THERON_BOOST
#elif THERON_CPP11
#elif defined(THERON_POSIX)
#endif
    }

    /**
    Returns a reference to the Mutex owned by the monitor.
    */
    THERON_FORCEINLINE Mutex &GetMutex()
    {
        return mMutex;
    }

    /**
    Suspends the calling thread until it is woken by another thread calling \ref Pulse or \ref PulseAll.
    \note The calling thread must hold a lock on the mutex associated with the condition.
    The lock owned by the caller is released, and automatically regained when the thread is woken.
    */
    THERON_FORCEINLINE void Wait(Lock &lock)
    {
#if THERON_WINDOWS

        lock.Unlock();

        // This waits until woken up by either of the events.
        THERON_ASSERT(mEvents[PULSE_EVENT]);
        THERON_ASSERT(mEvents[PULSE_ALL_EVENT]);
        WaitForMultipleObjects(
            2,                                  // Number of events to wait on
            mEvents,                            // Handles of the events
            false,                              // Don't wait for all events - one will do
            INFINITE);                          // No timeout

        lock.Relock();
    
#elif THERON_BOOST

        // The lock must be locked before calling Wait().
        THERON_ASSERT(lock.GetLock().owns_lock());
        mCondition.wait(lock.GetLock());

#elif THERON_CPP11
#elif defined(THERON_POSIX)
#endif
    }

    /**
    Wakes a single thread that is suspended after having called \ref Wait.
    */
    THERON_FORCEINLINE void Pulse()
    {
#if THERON_WINDOWS

        THERON_ASSERT(mEvents[PULSE_EVENT]);
        SetEvent(mEvents[PULSE_EVENT]);

#elif THERON_BOOST

        mCondition.notify_one();

#elif THERON_CPP11
#elif defined(THERON_POSIX)
#endif
    }

    /**
    Wakes all threads that are suspended after having called \ref Wait.
    */
    THERON_FORCEINLINE void PulseAll()
    {
#if THERON_WINDOWS

        THERON_ASSERT(mEvents[PULSE_ALL_EVENT]);
        SetEvent(mEvents[PULSE_ALL_EVENT]);

#elif THERON_BOOST

        mCondition.notify_all();

#elif THERON_CPP11
#elif defined(THERON_POSIX)
#endif
    }

private:

    Condition(const Condition &other);
    Condition &operator=(const Condition &other);
    
    Mutex mMutex;

#if THERON_WINDOWS

    HANDLE mEvents[2];

#elif THERON_BOOST

    boost::condition_variable mCondition;

#elif THERON_CPP11
#elif defined(THERON_POSIX)
#endif

};


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_THREADING_CONDITION_H
