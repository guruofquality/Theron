// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_DETAIL_THREADING_BOOST_MUTEX_H
#define THERON_DETAIL_THREADING_BOOST_MUTEX_H


#ifdef _MSC_VER
#pragma warning(push,0)
#endif //_MSC_VER

#include <boost/thread/mutex.hpp>

#ifdef _MSC_VER
#pragma warning(pop)
#endif //_MSC_VER

#include <Theron/Defines.h>


namespace Theron
{
namespace Detail
{


/// A simple critical section object implemented with Boost threads.
class Mutex
{
public:

    friend class Lock;

    /// Default constructor.
    THERON_FORCEINLINE Mutex() : mMutex()
    {
    }

    /// Locks the mutex, guaranteeing exclusive access to a protected resource associated with it.
    /// \note This is a blocking call and should be used with care to avoid deadlocks.
    THERON_FORCEINLINE void Lock()
    {
        mMutex.lock();
    }

    /// Unlocks the mutex, releasing exclusive access to a protected resource associated with it.
    THERON_FORCEINLINE void Unlock()
    {
        mMutex.unlock();
    }

private:

    /// Returns a reference to the owned mutex object.
    THERON_FORCEINLINE boost::mutex &GetMutex()
    {
        return mMutex;
    }

    Mutex(const Mutex &other);
    Mutex &operator=(const Mutex &other);

    boost::mutex mMutex;                    ///< Owned mutex object.
};


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_THREADING_BOOST_MUTEX_H
