// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_DETAIL_THREADING_STD_LOCK_H
#define THERON_DETAIL_THREADING_STD_LOCK_H


#ifdef _MSC_VER
#pragma warning(push,0)
#endif //_MSC_VER

#include <mutex> // includes unique_lock 

#ifdef _MSC_VER
#pragma warning(pop)
#endif //_MSC_VER

#include <Theron/Detail/Debug/Assert.h>
#include <Theron/Detail/Threading/Std/Mutex.h>
#include <Theron/Defines.h>


namespace Theron
{
namespace Detail
{


/// Object that locks a Mutex, implemented using std::thread.
/// \note This implementation uses C++0x threads, so requires C++0x compiler (e.g. GCC >= 4.6)
class Lock
{
public:

    friend class Monitor;

    /// Constructor.
    /// Creates a locked lock around the given mutex object.
    THERON_FORCEINLINE explicit Lock(Mutex &mutex) : mLock(mutex.GetMutex())
    {
    }

    /// Destructor.
    /// Unlocks the mutex prior to destruction.
    THERON_FORCEINLINE ~Lock()
    {
        THERON_ASSERT(mLock.owns_lock() == true);
        mLock.unlock();
    }

    /// Relocks the lock
    THERON_FORCEINLINE void Relock()
    {
        THERON_ASSERT(mLock.owns_lock() == false);
        mLock.lock();
    }

    /// Unlocks the lock
    THERON_FORCEINLINE void Unlock()
    {
        THERON_ASSERT(mLock.owns_lock() == true);
        mLock.unlock();
    }

private:

    Lock(const Lock &other);
    Lock &operator=(const Lock &other);

    /// Returns a reference to the wrapped std::unique_lock.
    THERON_FORCEINLINE std::unique_lock<std::mutex> &GetLock()
    {
        return mLock;
    }

    std::unique_lock<std::mutex> mLock;                 ///< Wrapped std::unique_lock.
};


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_THREADING_STD_LOCK_H

