// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_DEMOS_TOKENRING_TIMER_H
#define THERON_DEMOS_TOKENRING_TIMER_H


#ifdef _MSC_VER
#include <windows.h>
#elif defined(__GNUC__)
#include <sys/time.h>
#endif


// Simple timer class.
// Currently only implemented in Windows environments.
class Timer
{
public:

    Timer() : mSupported(false)
    {
#ifdef _MSC_VER
        // Read the counter frequency (in Hz) and an initial counter value.
        if (QueryPerformanceFrequency(&mTicksPerSecond) && QueryPerformanceCounter(&mCounterStartValue))
        {
            mSupported = true;
        }
#elif defined(__GNUC__)
        mSupported = true;
#endif
    }

    void Start()
    {
#ifdef _MSC_VER
        QueryPerformanceCounter(&mCounterStartValue);
#elif defined(__GNUC__)
        gettimeofday(&t1, NULL);
#endif
    }

    void Stop()
    {
#ifdef _MSC_VER
        QueryPerformanceCounter(&mCounterEndValue);
#elif defined(__GNUC__)
        gettimeofday(&t2, NULL);
#endif
    }

    bool Supported() const
    {
        return mSupported;
    }

    float Seconds() const
    {
#ifdef _MSC_VER
        const float elapsedTicks(static_cast<float>(mCounterEndValue.QuadPart - mCounterStartValue.QuadPart));
        const float ticksPerSecond(static_cast<float>(mTicksPerSecond.QuadPart));
        return (elapsedTicks / ticksPerSecond);
#elif defined(__GNUC__)
        return (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec) * 1e-6f;
#else
        return 0.0f;
#endif
    }

private:

    Timer(const Timer &other);
    Timer &operator=(const Timer &other);

    bool mSupported;

#ifdef _MSC_VER
    LARGE_INTEGER mTicksPerSecond;
    LARGE_INTEGER mCounterStartValue;
    LARGE_INTEGER mCounterEndValue;
#elif defined(__GNUC__)
    timeval t1, t2;
#endif

};


#endif // THERON_DEMOS_TOKENRING_TIMER_H

