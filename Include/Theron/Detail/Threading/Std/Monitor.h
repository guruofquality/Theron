// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_DETAIL_THREADING_STD_MONITOR_H
#define THERON_DETAIL_THREADING_STD_MONITOR_H


#ifdef _MSC_VER
#pragma warning(push,0)
#endif //_MSC_VER

#include <condition_variable>
#include <mutex>

#ifdef _MSC_VER
#pragma warning(pop)
#endif //_MSC_VER

#include <Theron/Detail/Debug/Assert.h>
#include <Theron/Detail/Threading/Std/Lock.h>
#include <Theron/Detail/Threading/Std/Mutex.h>
#include <Theron/Defines.h>


namespace Theron
{
namespace Detail
{


/// Monitor class implementing a C#/Java-style Monitor primitive.
/// \note This implementation uses C++0x threads, so requires a C++0x compiler (e.g. GCC >= 4.6)
class Monitor
{
public:

    /// Default constructor
    THERON_FORCEINLINE Monitor() :
      mMutex(),
      mCond()
    {
    }

    /// Returns a reference to the Mutex owned by the condition.
    THERON_FORCEINLINE Mutex &GetMutex()
    {
        return mMutex;
    }

    /// Waits for the monitor to be pulsed via Pulse or PulseAll.
    /// The calling thread should hold a lock on the mutex.
    /// The calling thread is blocked until another thread wakes it.
    /// The lock owned by the caller is released, and regained when the thread is woken.
    THERON_FORCEINLINE void Wait(Lock &lock)
    {
        // The lock must be locked, before calling Wait().
        THERON_ASSERT(lock.GetLock().owns_lock());
        mCond.wait(lock.GetLock());
    }

    /// Pulses the monitor, waking a single waiting thread.
    /// \note
    /// The calling thread should own a lock on the mutex. When the calling thread
    /// releases the lock, a woken thread aquires the lock and proceeds.
    THERON_FORCEINLINE void Pulse()
    {
        mCond.notify_one();
    }

    /// Pulses the monitor, waking all waiting threads.
    /// \note
    /// The calling thread should own the lock on the mutex. When the calling thread
    /// releases the lock, a woken thread aquires the lock and proceeds.
    THERON_FORCEINLINE void PulseAll()
    {
        mCond.notify_all();
    }

private:

    Monitor(const Monitor &other);
    Monitor &operator=(const Monitor &other);

    Mutex mMutex;                                   ///< Owned mutex.
    std::condition_variable mCond;                  ///< Owned condition variable.
};


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_THREADING_STD_MONITOR_H

