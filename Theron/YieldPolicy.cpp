// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#include <Theron/Assert.h>
#include <Theron/Defines.h>

#include <Theron/Detail/Scheduler/YieldPolicy.h>
#include <Theron/Detail/Threading/Utils.h>


namespace Theron
{
namespace Detail
{


void YieldPolicy::YieldPolite(const uint32_t counter)
{
    // This yield strategy scales from a simple 'nop' to putting the calling thread to sleep.
    // This implementation is based roughly on http://www.1024cores.net/home/lock-free-algorithms/tricks/spinning
    if (counter < 10)
    {
        Utils::YieldToHyperthread();
    }
    else if (counter < 20)
    {
        for (uint32_t i = 0; i < 50; ++i)
        {
            Utils::YieldToHyperthread();
        }
    }
    else if (counter < 22)
    {
        Utils::YieldToLocalThread();
    }
    else if (counter < 24)
    {
        Utils::YieldToAnyThread();
    }
    else
    {
        Utils::SleepThread(1UL);
    }
}


void YieldPolicy::YieldStrong(const uint32_t counter)
{
    // This 'strong' implementation yields after spinning for a while, but never sleeps.
    if (counter < 10)
    {
        Utils::YieldToHyperthread();
    }
    else if (counter < 20)
    {
        for (uint32_t i = 0; i < 50; ++i)
        {
            Utils::YieldToHyperthread();
        }
    }
    else if (counter < 22)
    {
        Utils::YieldToLocalThread();
    }
    else
    {
        Utils::YieldToAnyThread();
    }
}


void YieldPolicy::YieldAggressive(const uint32_t counter)
{
    // This 'aggressive' implementation never yields or sleeps.
    // It does however pause to allow another thread running on the same hyperthreaded core to proceed.
    if (counter < 10)
    {
        Utils::YieldToHyperthread();
    }
    else if (counter < 20)
    {
        for (uint32_t i = 0; i < 50; ++i)
        {
            Utils::YieldToHyperthread();
        }
    }
    else if (counter < 22)
    {
        for (uint32_t i = 0; i < 100; ++i)
        {
            Utils::YieldToHyperthread();
        }
    }
    else
    {
        for (uint32_t i = 0; i < 200; ++i)
        {
            Utils::YieldToHyperthread();
        }
    }
}


} // namespace Detail
} // namespace Theron


