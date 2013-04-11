// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_DETAIL_SCHEDULER_NONBLOCKINGSCHEDULER_H
#define THERON_DETAIL_SCHEDULER_NONBLOCKINGSCHEDULER_H


#include <Theron/Assert.h>
#include <Theron/BasicTypes.h>
#include <Theron/Defines.h>
#include <Theron/IAllocator.h>
#include <Theron/YieldStrategy.h>

#include <Theron/Detail/Scheduler/IScheduler.h>
#include <Theron/Detail/Scheduler/WorkQueue.h>
#include <Theron/Detail/Scheduler/YieldImplementation.h>
#include <Theron/Detail/Threading/SpinLock.h>


#ifdef _MSC_VER
#pragma warning(push)
#pragma warning (disable:4324)  // structure was padded due to __declspec(align())
#endif //_MSC_VER


namespace Theron
{
namespace Detail
{


class Mailbox;


/**
\brief Non-blocking scheduler implementation based on spinlocks.

Rather than traditional locks and condition variables, this scheduler is implemented
with spinlocks.

Traditional lock-based scheduling allows threads to be disabled
completely when not in use (by waiting on a condition variable) and then woken
specifically and individually when work arrives (by another thread pulsing the
condition variable). This results in efficient use of resources and so allows
high throughput, but suffers from high latency (as the waiting and pulsing of
threads is done via OS calls so take thousands of cycles) and scales badly
in highly parallel systems, particularly NUMA systems (because acquiring
locks and pulsing condition variables inevitably involves shared writes to
the same memory addresses by all the competing threads).

This implementation is based on spinlocks, which is a completely different
approach. It is called non-blocking because instead of making blocking
calls to OS functions to acquire locks and wait on condition variables,
threads simply spin in place waiting for work to arrive. This encourages
low latency (because waiting threads are always ready to go rather than being
thousands of cycles away deep in OS calls) and typically scales better on
highly parallel systems (because busy-waiting on a shared resource doesn't
involve writing to the resource so constitutes shared reads rather than
shared writes). Because it's bad form to spin indefinitely, the spinlocks
used by this scheduler allow smart spinning with progressive backoffs. The
backoff increases as the spincount increases, ranging from a simple NOP to
(optionally, and in the worst case) an OS sleep() call.

\see BlockingScheduler
*/
class NonBlockingScheduler : public IScheduler
{
public:

    NonBlockingScheduler();
    virtual ~NonBlockingScheduler();

    //
    // Accessor functions specific to this implementation.
    //

    void SetSharedWorkQueueSpinLock(SpinLock *const spinLock);
    void SetYieldStrategy(const YieldStrategy yieldStrategy);

    //
    // IScheduler interface implementation.
    //

    virtual void Initialize();
    virtual void Teardown();

    virtual void SetSharedWorkQueue(WorkQueue *const workQueue);
    virtual void SetLocalWorkQueue(WorkQueue *const workQueue);
    virtual WorkQueue *GetLocalWorkQueue();

    virtual void Push(Mailbox *const mailbox, const bool localQueue);
    virtual Mailbox *Pop();

private:

    NonBlockingScheduler(const NonBlockingScheduler &other);
    NonBlockingScheduler &operator=(const NonBlockingScheduler &other);

    SpinLock *mSharedWorkQueueSpinLock;             ///< Pointer to the spinlock protecting the shared work queue.
    WorkQueue *mSharedWorkQueue;                    ///< Pointer to the work queue shared by the threads in a framework.
    WorkQueue *mLocalWorkQueue;                     ///< Pointer to the local per-thread work queue.
    YieldImplementation mYield;                     ///< Thread yield strategy implementation.
};


} // namespace Detail
} // namespace Theron


#ifdef _MSC_VER
#pragma warning(pop)
#endif //_MSC_VER


#endif // THERON_DETAIL_SCHEDULER_NONBLOCKINGSCHEDULER_H
