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
#include <Theron/Detail/MailboxProcessor/ProcessorContext.h>


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
    inline static bool Send(
        EndPoint *const endPoint,
        ProcessorContext *const processorContext,
        const uint32_t localFrameworkIndex,
        IMessage *const message,
        const Address &address);

    /**
    Delivers an allocated message to a receiver or an actor in some framework within this process.
    */
    inline static bool DeliverToLocalMailbox(
        IMessage *const message,
        const Address &address);

private:

    /**
    Sends an allocated message to an entity addressed only by name.
    This function is non-inlined so serves as a call-point to avoid excessive inlining.
    */
    static bool DeliverByName(
        EndPoint *const endPoint,
        ProcessorContext *const processorContext,
        const uint32_t localFrameworkIndex,
        IMessage *const message,
        const String &name);

    /**
    Sends an allocated message to a local entity addressed by a known local index
    This function is non-inlined so serves as a call-point to avoid excessive inlining.
    */
    static bool DeliverByIndex(
        ProcessorContext *const processorContext,
        const uint32_t localFrameworkIndex,
        IMessage *const message,
        const Address &address);

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
    static bool DeliverToActorInLocalFramework(
        IMessage *const message,
        const Address &address);
};


THERON_FORCEINLINE bool MessageSender::Send(
    EndPoint *const endPoint,
    ProcessorContext *const processorContext,
    const uint32_t localFrameworkIndex,
    IMessage *const message,
    const Address &address)
{
    // Index of zero implies the actor is addressed only by name and may be remote.
    if (address.mIndex.mUInt32 == 0)
    {
        // Non-inlined call.
        return DeliverByName(
            endPoint,
            processorContext,
            localFrameworkIndex,
            message,
            address.GetName());
    }

    // Non-inlined call.
    return DeliverByIndex(
        processorContext,
        localFrameworkIndex,
        message,
        address);
}


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


THERON_FORCEINLINE bool MessageSender::DeliverToLocalMailbox(IMessage *const message, const Address &address)
{
    const Index index(address.mIndex);
    const uint32_t targetFrameworkIndex(index.mComponents.mFramework);
    
    THERON_ASSERT(index.mUInt32 != 0);

    // Is the message addressed to a receiver? Receiver addresses have zero framework indices.
    if (targetFrameworkIndex == 0)
    {
        return DeliverToReceiver(
            message,
            address);
    }

    return DeliverToActorInLocalFramework(
        message,
        address);
}


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_MESSAGES_MESSAGESENDER_H
