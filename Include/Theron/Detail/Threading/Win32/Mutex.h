// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_DETAIL_THREADING_WIN32_MUTEX_H
#define THERON_DETAIL_THREADING_WIN32_MUTEX_H


#ifdef _MSC_VER
#pragma warning(push,0)
#endif //_MSC_VER

#include <windows.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif //_MSC_VER

#include <Theron/Defines.h>


namespace Theron
{
namespace Detail
{


/// A simple critical section object implemented with Win32 threads.
class Mutex
{
public:

    /// Default constructor.
    THERON_FORCEINLINE Mutex()
    {
        InitializeCriticalSection(&mCriticalSection);
    }

    /// Destructor.
    THERON_FORCEINLINE ~Mutex()
    {
        DeleteCriticalSection(&mCriticalSection);
    }

    /// Locks the mutex, guaranteeing exclusive access to a protected resource associated with it.
    /// \note This is a blocking call and should be used with care to avoid deadlocks.
    THERON_FORCEINLINE void Lock()
    {
        EnterCriticalSection(&mCriticalSection);
    }

    /// Unlocks the mutex, releasing exclusive access to a protected resource associated with it.
    THERON_FORCEINLINE void Unlock()
    {
        LeaveCriticalSection(&mCriticalSection);
    }

private:

    Mutex(const Mutex &other);
    Mutex &operator=(const Mutex &other);

    CRITICAL_SECTION mCriticalSection;  ///< Owned Win32 critical section object.
};


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_THREADING_WIN32_MUTEX_H
