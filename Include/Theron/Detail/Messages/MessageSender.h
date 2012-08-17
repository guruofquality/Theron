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
#include <Theron/Detail/Mailboxes/Queue.h>
#include <Theron/Detail/MailboxProcessor/ProcessorContext.h>


namespace Theron
{
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
    This function is non-inlined so serves mainly as a call-point to avoid excessive inlining.
    It also helps to break cyclic include dependencies by moving the implementation into the source file.
    */
    static bool Send(
        ProcessorContext *const processorContext,
        const uint32_t localFrameworkIndex,
        IMessage *const message,
        const Address &address);

private:

    /**
    Sends an allocated message to a receiver in this process.
    */
    inline static bool DeliverToReceiver(
        IMessage *const message,
        const Address &address);

    /**
    Sends an allocated message to an actor in this framework.
    */
    inline static bool DeliverToActorInThisFramework(
        ProcessorContext *const processorContext,
        IMessage *const message,
        const Address &address);

    /**
    Sends an allocated message to an actor in a different framework within this process.
    \note This function is intentionally not inlined to reduce the codesize of the common case.
    */
    static bool DeliverToActorInAnotherFramework(
        IMessage *const message,
        const Address &address);
};


THERON_FORCEINLINE bool MessageSender::DeliverToReceiver(
    IMessage *const message,
    const Address &address)
{
    // Get a reference to the receiver directory entry for this address.
    Entry &entry(StaticDirectory<Receiver>::GetEntry(address.AsInteger()));

    // Pin the entry and lookup the entity registered at the address.
    entry.Lock();
    entry.Pin();
    Receiver *const receiver(static_cast<Receiver *>(entry.GetEntity()));
    entry.Unlock();

    // If a receiver is registered at the mailbox then deliver the message to it.
    if (receiver)
    {
        receiver->Push(message);
    }

    // Unpin the entry, allowing it to be changed by other threads.
    entry.Lock();
    entry.Unpin();
    entry.Unlock();

    return (receiver != 0);
}


THERON_FORCEINLINE bool MessageSender::DeliverToActorInThisFramework(
    ProcessorContext *const processorContext,
    IMessage *const message,
    const Address &address)
{
    // Message is addressed to an actor, which we assume for now is in this framework.
    // Get a reference to the destination mailbox.
    Mailbox &mailbox(processorContext->mMailboxes->GetEntry(address.AsInteger()));

    // Push the message into the mailbox and schedule the mailbox for processing
    // if it was previously empty, so won't already be scheduled.
    // The message will be destroyed by the worker thread that does the processing,
    // even if it turns out that no actor is registered with the mailbox.
    mailbox.Lock();

    const bool schedule(mailbox.Empty());
    mailbox.Push(message);

    if (schedule)
    {
        processorContext->mWorkQueue->Push(&mailbox);
    }

    mailbox.Unlock();

    return true;
}


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_MESSAGES_MESSAGESENDER_H
