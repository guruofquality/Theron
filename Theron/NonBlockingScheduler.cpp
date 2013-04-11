// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#include <Theron/Assert.h>
#include <Theron/Defines.h>

#include <Theron/Detail/Mailboxes/Mailbox.h>
#include <Theron/Detail/Scheduler/NonBlockingScheduler.h>
#include <Theron/Detail/Scheduler/YieldPolicy.h>


namespace Theron
{
namespace Detail
{


NonBlockingScheduler::NonBlockingScheduler() :
  mSharedWorkQueueSpinLock(0),
  mSharedWorkQueue(0),
  mLocalWorkQueue(0),
  mYield()
{
}


NonBlockingScheduler::~NonBlockingScheduler()
{
}


void NonBlockingScheduler::SetSharedWorkQueueSpinLock(SpinLock *const spinLock)
{
    mSharedWorkQueueSpinLock = spinLock;
}


void NonBlockingScheduler::SetYieldStrategy(const YieldStrategy yieldStrategy)
{
    switch (yieldStrategy)
    {
        case YIELD_STRATEGY_POLITE:     mYield.SetYieldFunction(&Detail::YieldPolicy::YieldPolite);       break;
        case YIELD_STRATEGY_STRONG:     mYield.SetYieldFunction(&Detail::YieldPolicy::YieldStrong);       break;
        case YIELD_STRATEGY_AGGRESSIVE: mYield.SetYieldFunction(&Detail::YieldPolicy::YieldAggressive);   break;
        default:                        mYield.SetYieldFunction(&Detail::YieldPolicy::YieldPolite);       break;
    }
}


void NonBlockingScheduler::SetSharedWorkQueue(WorkQueue *const workQueue)
{
    mSharedWorkQueue = workQueue;
}


void NonBlockingScheduler::SetLocalWorkQueue(WorkQueue *const workQueue)
{
    mLocalWorkQueue = workQueue;
}


WorkQueue *NonBlockingScheduler::GetLocalWorkQueue()
{
    return mLocalWorkQueue;
}


void NonBlockingScheduler::Initialize()
{
}


void NonBlockingScheduler::Teardown()
{
}


void NonBlockingScheduler::Push(Mailbox *const mailbox, const bool localQueue)
{
    // Push the receiving mailbox onto either the shared or local work queue.
    // If the current context isn't a per-thread context then it may not have a local queue.
    if (localQueue && mLocalWorkQueue)
    {
        // The local queue in a per-thread context is only accessed by that thread
        // so we don't need to protect access to it.
        mLocalWorkQueue->Push(mailbox);
    }
    else
    {
        // Because the shared work queue is accessed by multiple threads we have to protect it.
        mSharedWorkQueueSpinLock->Lock();
        mSharedWorkQueue->Push(mailbox);
        mSharedWorkQueueSpinLock->Unlock();
    }
}


Mailbox *NonBlockingScheduler::Pop()
{
    Mailbox *mailbox(0);

    // Try the local queue first.
    // We only check the shared queue once the local queue is empty.
    if (mLocalWorkQueue)
    {
        mailbox = static_cast<Mailbox *>(mLocalWorkQueue->Pop());
        if (mailbox)
        {
            mYield.Reset();
            return mailbox;
        }
    }

    // Try the shared queue.
    mSharedWorkQueueSpinLock->Lock();
    mailbox = static_cast<Mailbox *>(mSharedWorkQueue->Pop());
    mSharedWorkQueueSpinLock->Unlock();

    if (mailbox)
    {
        mYield.Reset();
        return mailbox;
    }

    // Progressive backoff.
    mYield.Execute();

    return 0;
}


} // namespace Detail
} // namespace Theron

