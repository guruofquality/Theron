// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_DETAIL_MAILBOXPROCESSOR_WORKITEM_H
#define THERON_DETAIL_MAILBOXPROCESSOR_WORKITEM_H


#include <Theron/Address.h>
#include <Theron/Defines.h>

#include <Theron/Detail/Mailboxes/Mailbox.h>
#include <Theron/Detail/Mailboxes/Queue.h>


namespace Theron
{
namespace Detail
{


struct ProcessorContext;


/**
Mailbox processor.
*/
class WorkItem
{
public:

    /**
    Processes the work item, re-adding it to the given work queue if it needs more processing.
    */
    static void Process(Mailbox *const mailbox, ProcessorContext *const context);

private:

    WorkItem();
    WorkItem(const WorkItem &other);
    WorkItem &operator=(const WorkItem &other);
};


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_MAILBOXPROCESSOR_WORKITEM_H
