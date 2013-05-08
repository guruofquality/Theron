// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_DETAIL_SCHEDULER_COUNTING_H
#define THERON_DETAIL_SCHEDULER_COUNTING_H


#include <Theron/Defines.h>

#include <Theron/Detail/Threading/Atomic.h>
#include <Theron/Detail/Threading/Utils.h>


#ifndef THERON_COUNTER_INCREMENT
#if THERON_ENABLE_COUNTERS
#define THERON_COUNTER_INCREMENT(counter) counter.Increment()
#define THERON_COUNTER_RESET(counter) counter.Store(0)
#define THERON_COUNTER_QUERY(counter) counter.Load()
#define THERON_COUNTER_RAISE(counter, n) Theron::Detail::RaiseCounter(counter, n)
#else
#define THERON_COUNTER_INCREMENT(counter) (void)counter
#define THERON_COUNTER_RESET(counter) (void)counter
#define THERON_COUNTER_QUERY(counter) counter.Load()
#define THERON_COUNTER_RAISE(counter, n) (void)counter; (void)n
#endif // THERON_ENABLE_COUNTERS
#endif // THERON_COUNTER_INCREMENT


namespace Theron
{
namespace Detail
{


THERON_FORCEINLINE void RaiseCounter(Atomic::UInt32 &counter, const uint32_t n)
{
    uint32_t currentValue(counter.Load());
    uint32_t backoff(0);

    while (n > currentValue)
    {
        if (counter.CompareExchangeAcquire(currentValue, n))
        {
            break;
        }

        Utils::Backoff(backoff);
    }
}


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_SCHEDULER_COUNTING_H
