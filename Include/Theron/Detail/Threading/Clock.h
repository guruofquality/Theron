// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_DETAIL_THREADING_CLOCK_H
#define THERON_DETAIL_THREADING_CLOCK_H


#include <Theron/Defines.h>
#include <Theron/BasicTypes.h>


#if THERON_MSVC
#pragma warning(push,0)
#endif // THERON_MSVC

#if THERON_WINDOWS
#include <windows.h>
#elif THERON_POSIX
#include <time.h>
#elif THERON_GCC
#include <sys/time.h>
#endif

#if THERON_MSVC
#pragma warning(pop)
#endif // THERON_MSVC


namespace Theron
{
namespace Detail
{


/**
Static helper class that queries system performance timers.
*/
class Clock
{
public:

    /**
    Queries the clock for a timestamp in ticks.
    The time-length of a tick is implementation-dependent.
    */
    THERON_FORCEINLINE static bool GetTicks(uint64_t &ticks)
    {
#if THERON_WINDOWS

        // The 'ticks' are cycles in the Windows implementation.
        LARGE_INTEGER counter;
        if (QueryPerformanceCounter(&counter))
        {
            ticks = (uint64_t) counter.QuadPart;
            return true;
        }

#elif THERON_POSIX

        // The 'ticks' are nanoseconds in the POSIX implementation.
        // TODO: Use CLOCK_PROCESS_CPUTIME_ID?
        struct timespec ts;
        if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0)
        {
            ticks = ts.tv_sec * NANOSECONDS_PER_SECOND + ts.tv_nsec;
            return true;
        }

#elif THERON_GCC

        // The 'ticks' are nanoseconds in the GCC implementation.
        timeval td;
        if (gettimeofday(&td, NULL))
        {
            ticks = td.tv_sec * NANOSECONDS_PER_SECOND + t1.tv_usec * NANOSECONDS_PER_MICROSECOND;
            return true;
        }

#endif

        return false;
    }

    /**
    Queries the clock for its resolution in ticks-per-second.
    */
    THERON_FORCEINLINE static bool GetFrequency(uint64_t &ticksPerSecond)
    {
#if THERON_WINDOWS

        // The 'ticks' are cycles in the Windows implementation.
        LARGE_INTEGER counter;
        if (QueryPerformanceFrequency(&counter))
        {
            ticksPerSecond = (uint64_t) counter.QuadPart;
            return true;
        }

        return false;

#elif THERON_POSIX

        // The 'ticks' are nanoseconds in the POSIX implementation.
        ticksPerSecond = NANOSECONDS_PER_SECOND;
        return true;

#elif THERON_GCC

        // The 'ticks' are nanoseconds in the GCC implementation.
        ticksPerSecond = NANOSECONDS_PER_SECOND;
        return true;

#else

        return false;

#endif

    }

    static const uint64_t NANOSECONDS_PER_SECOND = 1000000000;
    static const uint64_t NANOSECONDS_PER_MICROSECOND = 1000;
};


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_THREADING_CLOCK_H
