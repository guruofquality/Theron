// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_DETAIL_THREADING_BOOST_LOCK_H
#define THERON_DETAIL_THREADING_BOOST_LOCK_H


#ifdef _MSC_VER
#pragma warning(push,0)
#endif //_MSC_VER

#include <boost/thread/locks.hpp>
#include <boost/thread/recursive_mutex.hpp>

#ifdef _MSC_VER
#pragma warning(pop)
#endif //_MSC_VER

#include <Theron/Detail/Debug/Assert.h>
#include <Theron/Detail/Threading/Boost/Mutex.h>

#include <Theron/Defines.h>


namespace Theron
{
namespace Detail
{


/// Object that locks a Mutex, implemented using Boost threads.
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

    /// Returns a reference to the wrapped boost::unique_lock.
    THERON_FORCEINLINE boost::unique_lock<boost::recursive_mutex> &GetLock()
    {
        return mLock;
    }

    boost::unique_lock<boost::recursive_mutex> mLock;       ///< Wrapped boost::unique_lock.
};


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_THREADING_BOOST_LOCK_H

