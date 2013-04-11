// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_DETAIL_SCHEDULER_BLOCKINGSCHEDULER_H
#define THERON_DETAIL_SCHEDULER_BLOCKINGSCHEDULER_H


#include <Theron/Assert.h>
#include <Theron/BasicTypes.h>
#include <Theron/Defines.h>
#include <Theron/IAllocator.h>

#include <Theron/Detail/Scheduler/IScheduler.h>
#include <Theron/Detail/Scheduler/WorkQueue.h>
#include <Theron/Detail/Threading/Condition.h>
#include <Theron/Detail/Threading/Lock.h>
#include <Theron/Detail/Threading/Mutex.h>


namespace Theron
{
namespace Detail
{


class Mailbox;


/**
\brief Blocking scheduler implementation based on a condition variable.

\see NonBlockingScheduler
*/
class BlockingScheduler : public IScheduler
{
public:

    BlockingScheduler();
    virtual ~BlockingScheduler();

    //
    // Accessor functions specific to this implementation.
    //

    void SetSharedWorkQueueCondition(Condition *const condition);

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

    BlockingScheduler(const BlockingScheduler &other);
    BlockingScheduler &operator=(const BlockingScheduler &other);

    Condition *mSharedWorkQueueCondition;           ///< Pointer to the shared work queue condition variable.
    WorkQueue *mSharedWorkQueue;                    ///< Pointer to the work queue shared by the threads in a framework.
    WorkQueue *mLocalWorkQueue;                     ///< Pointer to the local per-thread work queue.
};


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_SCHEDULER_BLOCKINGSCHEDULER_H
