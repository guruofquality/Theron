// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#include <Theron/Assert.h>
#include <Theron/Defines.h>

#include <Theron/Detail/Mailboxes/Mailbox.h>
#include <Theron/Detail/Scheduler/BlockingScheduler.h>


namespace Theron
{
namespace Detail
{


BlockingScheduler::BlockingScheduler() :
  mSharedWorkQueueCondition(0),
  mSharedWorkQueue(0),
  mLocalWorkQueue(0)
{
}


BlockingScheduler::~BlockingScheduler()
{
}


void BlockingScheduler::SetSharedWorkQueueCondition(Condition *const condition)
{
    mSharedWorkQueueCondition = condition;
}


void BlockingScheduler::SetSharedWorkQueue(WorkQueue *const workQueue)
{
    mSharedWorkQueue = workQueue;
}


void BlockingScheduler::SetLocalWorkQueue(WorkQueue *const workQueue)
{
    mLocalWorkQueue = workQueue;
}


WorkQueue *BlockingScheduler::GetLocalWorkQueue()
{
    return mLocalWorkQueue;
}


void BlockingScheduler::Initialize()
{
}


void BlockingScheduler::Teardown()
{
}


void BlockingScheduler::Push(Mailbox *const mailbox, const bool localQueue)
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
        Lock lock(mSharedWorkQueueCondition->GetMutex());

        mSharedWorkQueue->Push(mailbox);
        lock.Unlock();
        mSharedWorkQueueCondition->Pulse();
    }
}


Mailbox *BlockingScheduler::Pop()
{
    Mailbox *mailbox(0);

    // Try the local queue first.
    // We only check the shared queue once the local queue is empty.
    if (mLocalWorkQueue)
    {
        mailbox = static_cast<Mailbox *>(mLocalWorkQueue->Pop());
        if (mailbox)
        {
            return mailbox;
        }
    }

    // Try the shared queue.
    {
        Lock lock(mSharedWorkQueueCondition->GetMutex());

        mailbox = static_cast<Mailbox *>(mSharedWorkQueue->Pop());
        if (mailbox == 0)
        {
            // Wait to be pulsed when work arrives on the shared queue.
            mSharedWorkQueueCondition->Wait(lock);
        }
    }

    return mailbox;
}


} // namespace Detail
} // namespace Theron

