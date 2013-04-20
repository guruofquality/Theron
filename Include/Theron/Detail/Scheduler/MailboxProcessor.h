// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_DETAIL_SCHEDULER_MAILBOXPROCESSOR_H
#define THERON_DETAIL_SCHEDULER_MAILBOXPROCESSOR_H


#include <Theron/BasicTypes.h>
#include <Theron/Defines.h>

#include <Theron/Detail/Mailboxes/Mailbox.h>
#include <Theron/Detail/Scheduler/WorkerContext.h>


namespace Theron
{
namespace Detail
{


/**
Mailbox processor.
*/
class MailboxProcessor
{
public:

    static void Process(WorkerContext *const workerContext, Mailbox *const mailbox);

private:

    MailboxProcessor();
    MailboxProcessor(const MailboxProcessor &other);
    MailboxProcessor &operator=(const MailboxProcessor &other);
};


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_SCHEDULER_MAILBOXPROCESSOR_H
