// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_DETAIL_SCHEDULER_SCHEDULER_H
#define THERON_DETAIL_SCHEDULER_SCHEDULER_H


#include <Theron/BasicTypes.h>
#include <Theron/Defines.h>

#include <Theron/Detail/Mailboxes/Mailbox.h>
#include <Theron/Detail/Scheduler/MailboxContext.h>


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

    static void ProcessMailbox(MailboxContext *const context, Mailbox *const mailbox);

private:

    MailboxProcessor();
    MailboxProcessor(const MailboxProcessor &other);
    MailboxProcessor &operator=(const MailboxProcessor &other);
};


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_SCHEDULER_SCHEDULER_H
