// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_DETAIL_SCHEDULER_ISCHEDULER_H
#define THERON_DETAIL_SCHEDULER_ISCHEDULER_H


#include <Theron/BasicTypes.h>
#include <Theron/Defines.h>


namespace Theron
{
namespace Detail
{


struct MailboxContext;
class Mailbox;
class WorkQueue;


/**
Generic mailbox scheduler interface.
*/
class IScheduler
{
public:

    /**
    Default constructor.
    */
    IScheduler()
    {
    }

    /**
    Virtual destructor.
    */
    virtual ~IScheduler()
    {
    }

    virtual void SetSharedWorkQueue(WorkQueue *const workQueue) = 0;
    virtual void SetLocalWorkQueue(WorkQueue *const workQueue) = 0;
    virtual WorkQueue *GetLocalWorkQueue() = 0;

    virtual void Initialize() = 0;
    virtual void Teardown() = 0;

    virtual Mailbox *Pop() = 0;
    virtual void Push(Mailbox *const mailbox, const bool localQueue) = 0;

private:

    IScheduler(const IScheduler &other);
    IScheduler &operator=(const IScheduler &other);
};


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_SCHEDULER_ISCHEDULER_H
