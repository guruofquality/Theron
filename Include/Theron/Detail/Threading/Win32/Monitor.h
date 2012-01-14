// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_DETAIL_THREADING_WIN32_MONITOR_H
#define THERON_DETAIL_THREADING_WIN32_MONITOR_H


#ifdef _MSC_VER
#pragma warning(push,0)
#endif //_MSC_VER

#include <windows.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif //_MSC_VER

#include <Theron/Detail/BasicTypes.h>
#include <Theron/Detail/Debug/Assert.h>
#include <Theron/Detail/Threading/Win32/Lock.h>
#include <Theron/Detail/Threading/Win32/Mutex.h>

#include <Theron/Defines.h>


namespace Theron
{
namespace Detail
{


/// Implements a monitor/condition object using Win32 threads.
/// The monitor essentially ties an event to a mutex, allowing a
/// thread to Wait until it is Pulsed when the mutex becomes available.
class Monitor
{
public:

    enum
    {
        PULSE_EVENT = 0,        ///< Event used to wake a single waiting thread.
        PULSE_ALL_EVENT = 1     ///< Event used to wake all waiting threads.
    };

    /// Default constructor
    inline Monitor();

    /// Destructor
    /// \note If the monitor is destroyed while threads are still waiting on it, the result is undefined.
    inline ~Monitor();

    /// Returns a reference to the Mutex owned by the monitor.
    THERON_FORCEINLINE Mutex &GetMutex()
    {
        return mMutex;
    }

    /// Waits for the monitor to be pulsed via Pulse or PulseAll.
    /// The calling thread should hold a lock on the mutex.
    /// The calling thread is blocked until another thread wakes it.
    /// The lock owned by the caller is released, and regained when the thread is woken.
    inline void Wait(Lock &lock);

    /// Pulses the monitor, waking a single waiting thread.
    /// \note
    /// The calling thread should own a lock on the mutex. When the calling thread
    /// releases the lock, a woken thread aquires the lock and proceeds.
    inline void Pulse();

    /// Pulses the monitor, waking all waiting threads.
    /// \note
    /// The calling thread should own the lock on the mutex. When the calling thread
    /// releases the lock, a woken thread aquires the lock and proceeds.
    inline void PulseAll();

private:

    Monitor(const Monitor &other);
    Monitor &operator=(const Monitor &other);
    
    Mutex mMutex;           ///< A critical section object used to guarantee exclusive access.
    HANDLE mEvents[2];      ///< Win32 events used to wake waiting threads.
};


THERON_FORCEINLINE Monitor::Monitor() : mMutex()
{
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
}


THERON_FORCEINLINE Monitor::~Monitor()
{
    THERON_ASSERT(mEvents[PULSE_EVENT]);
    THERON_ASSERT(mEvents[PULSE_ALL_EVENT]);
    CloseHandle(mEvents[PULSE_EVENT]);
    CloseHandle(mEvents[PULSE_ALL_EVENT]);
}


THERON_FORCEINLINE void Monitor::Wait(Lock &lock)
{
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
}


THERON_FORCEINLINE void Monitor::Pulse()
{
    THERON_ASSERT(mEvents[PULSE_EVENT]);
    SetEvent(mEvents[PULSE_EVENT]);
}


THERON_FORCEINLINE void Monitor::PulseAll()
{
    THERON_ASSERT(mEvents[PULSE_ALL_EVENT]);
    SetEvent(mEvents[PULSE_ALL_EVENT]);
}


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_THREADING_WIN32_MONITOR_H

