// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#include <Theron/Defines.h>

// Must include xs.h before standard headers to avoid warnings in MS headers!
#if THERON_XS
#include <xs/xs.h>
#endif // THERON_XS

#include <Theron/EndPoint.h>
#include <Theron/Framework.h>

#include <Theron/Detail/Directory/StaticDirectory.h>
#include <Theron/Detail/Messages/MessageCreator.h>
#include <Theron/Detail/Messages/MessageSender.h>
#include <Theron/Detail/Network/Index.h>
#include <Theron/Detail/Network/String.h>


namespace Theron
{
namespace Detail
{


bool MessageSender::Send(
    EndPoint *const endPoint,
    Processor::Context *const processorContext,
    const uint32_t localFrameworkIndex,
    IMessage *const message,
    const Address &address,
    const bool localQueue)
{
    boost::unique_lock<boost::recursive_mutex> lock(processorContext->mutex);
    Index index(address.mIndex);

    // Index of zero implies the actor is addressed only by name and may be remote.
    if (index.mUInt32 == 0)
    {
        const String &name(address.GetName());

        THERON_ASSERT(!name.IsNull());
        THERON_ASSERT_MSG(endPoint, "Sending messages addressed by name requires a Theron::EndPoint");

        // Search the local endPoint for a mailbox with the given name.
        // If there is a local match we fall through using the retrieved index, and we don't
        // bother to push the message out onto the network, since names are globally unique.
        if (!endPoint->Lookup(name, index))
        {
            // If there isn't a local match we send the message out onto the network.
            const bool r = endPoint->RequestSend(message, name);
            processorContext->cond.notify_one();
            return r;
        }
    }

    // Which framework is the addressed entity in?
    const uint32_t targetFrameworkIndex(index.mComponents.mFramework);
    if (targetFrameworkIndex == localFrameworkIndex)
    {
        // Message is addressed to an actor in the sending framework.
        // Get a reference to the destination mailbox.
        Mailbox &mailbox(processorContext->mMailboxes->GetEntry(index.mComponents.mIndex));

        // Push the message into the mailbox and schedule the mailbox for processing
        // if it was previously empty, so won't already be scheduled.
        // The message will be destroyed by the worker thread that does the processing,
        // even if it turns out that no actor is registered with the mailbox.
        mailbox.Lock();

        const bool schedule(mailbox.Empty());
        mailbox.Push(message);

        if (schedule)
        {
            // Push the receiving mailbox onto either the shared or local work queue.
            // If the current context isn't a per-thread context then it may not have a local queue.
            if (localQueue && processorContext->mLocalWorkQueue)
            {
                // The local queue in a per-thread context is only accessed by that thread
                // so we don't need to protect access to it.
                processorContext->mLocalWorkQueue->Push(&mailbox);
            }
            else
            {
                // Because the shared work queue is accessed by multiple threads we have to protect it.
                processorContext->mSharedWorkQueueSpinLock->Lock();
                processorContext->mSharedWorkQueue->Push(&mailbox);
                processorContext->mSharedWorkQueueSpinLock->Unlock();
            }
        }

        mailbox.Unlock();

            processorContext->cond.notify_one();
        return true;
    }

    // Message is addressed to a mailbox in the local process but not in the
    // sending Framework. In this less common case we pay the hit of an extra call.
    if (DeliverWithinLocalProcess(message, index))
    {
            processorContext->cond.notify_one();
        return true;
    }

    // Destroy the undelivered message.
    processorContext->mFallbackHandlers->Handle(message);
    Detail::MessageCreator::Destroy(&processorContext->mMessageCache, message);

            processorContext->cond.notify_one();
    return false;
}


bool MessageSender::DeliverWithinLocalProcess(IMessage *const message, const Index &index)
{
    const uint32_t targetFrameworkIndex(index.mComponents.mFramework);

    THERON_ASSERT(index.mUInt32 != 0);

    // Is the message addressed to a receiver? Receiver addresses have zero framework indices.
    if (targetFrameworkIndex == 0)
    {
        // Get a reference to the receiver directory entry for this address.
        Entry &entry(StaticDirectory<Receiver>::GetEntry(index.mComponents.mIndex));

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

    bool delivered(false);

    // TODO: Return a pointer so we can handle missing pages gracefully.
    // Get the entry for the addressed framework.
    Entry &entry(StaticDirectory<Framework>::GetEntry(index.mComponents.mFramework));

    // Pin the entry and lookup the framework registered at the index.
    entry.Lock();
    entry.Pin();
    Framework *const framework(static_cast<Framework *>(entry.GetEntity()));
    entry.Unlock();

    // If a framework is registered at this index then forward the message to it.
    if (framework)
    {
        // The address is just an index with no name.
        const Address address(String(), index);
        delivered = framework->FrameworkReceive(message, address);
    }

    // Unpin the entry, allowing it to be changed by other threads.
    entry.Lock();
    entry.Unpin();
    entry.Unlock();

    return delivered;
}


} // namespace Detail
} // namespace Theron


