// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_DETAIL_THREADING_WIN32_LOCK_H
#define THERON_DETAIL_THREADING_WIN32_LOCK_H


#include <Theron/Detail/Debug/Assert.h>
#include <Theron/Detail/Threading/Win32/Mutex.h>

#include <Theron/Defines.h>


namespace Theron
{
namespace Detail
{


/// Object that locks a Mutex, implemented using Win32 threads.
class Lock
{
public:

    /// Constructor.
    /// Creates a locked lock around the given mutex object.
    THERON_FORCEINLINE explicit Lock(Mutex &mutex) : mMutex(mutex)
    {
        mMutex.Lock();
    }

    /// Destructor.
    /// Unlocks the mutex prior to destruction.
    THERON_FORCEINLINE ~Lock()
    {
        mMutex.Unlock();
    }

    /// Relocks the lock
    THERON_FORCEINLINE void Relock()
    {
        mMutex.Lock();
    }

    /// Unlocks the lock
    THERON_FORCEINLINE void Unlock()
    {
        mMutex.Unlock();
    }

private:

    Lock(const Lock &other);
    Lock &operator=(const Lock &other);

    Mutex &mMutex;              ///< Referenced Mutex object.
};


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_THREADING_WIN32_LOCK_H

