// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_DETAIL_MESSAGES_MESSAGESENDER_H
#define THERON_DETAIL_MESSAGES_MESSAGESENDER_H


#include <Theron/Address.h>
#include <Theron/Assert.h>
#include <Theron/BasicTypes.h>
#include <Theron/Defines.h>
#include <Theron/IAllocator.h>
#include <Theron/Receiver.h>

#include <Theron/Detail/Directory/Entry.h>
#include <Theron/Detail/Directory/StaticDirectory.h>
#include <Theron/Detail/Handlers/FallbackHandlerCollection.h>
#include <Theron/Detail/Messages/IMessage.h>
#include <Theron/Detail/Mailboxes/Mailbox.h>
#include <Theron/Detail/Scheduler/MailboxContext.h>


namespace Theron
{


class EndPoint;


namespace Detail
{


/**
Helper class that sends allocated internal message objects.
*/
class MessageSender
{
public:

    /**
    Sends an allocated message to the given address.
    */
    static bool Send(
        EndPoint *const endPoint,
        MailboxContext *const mailboxContext,
        const uint32_t localFrameworkIndex,
        IMessage *const message,
        const Address &address,
        const bool localQueue = false);

    /**
    Delivers an allocated message to a receiver or an actor in some framework within this process.
    This function is non-inlined so serves as a call-point to avoid excessive inlining.
    */
    static bool DeliverWithinLocalProcess(
        IMessage *const message,
        const Index &index);
};


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_MESSAGES_MESSAGESENDER_H
