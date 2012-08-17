// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#include <Theron/Framework.h>

#include <Theron/Detail/Directory/StaticDirectory.h>
#include <Theron/Detail/Messages/MessageCreator.h>
#include <Theron/Detail/Messages/MessageSender.h>


namespace Theron
{
namespace Detail
{


bool MessageSender::Send(
    ProcessorContext *const processorContext,
    const uint32_t localFrameworkIndex,
    IMessage *const message,
    const Address &address)
{
    // For now we assume that the message is addressed to an entity in the local process
    // on the local host. Other processes and hosts aren't supported yet.
    THERON_ASSERT(address.GetHost() == 0);
    THERON_ASSERT(address.GetProcess() == 0);

    // Which framework is the addressed entity in?
    const uint32_t targetFrameworkIndex(address.GetFramework());

    bool delivered(false);
    if (targetFrameworkIndex == localFrameworkIndex)
    {
        // Is the message addressed to an actor in this framework?
        delivered = DeliverToActorInThisFramework(
            processorContext,
            message,
            address);
    }
    else if (targetFrameworkIndex == 0)
    {
        // Is the message addressed to a receiver? Receiver addresses have zero framework indices.
        delivered = DeliverToReceiver(
            message,
            address);
    }
    else
    {
        // The message must be addressed to an actor in a different framework.
        delivered = DeliverToActorInAnotherFramework(
            message,
            address);
    }

    if (delivered)
    {
        return true;
    }

    // Destroy the undelivered message.
    processorContext->mFallbackHandlers->Handle(message);
    Detail::MessageCreator::Destroy(processorContext->mMessageAllocator, message);

    return false;
}


bool MessageSender::DeliverToActorInAnotherFramework(
    IMessage *const message,
    const Address &address)
{
    bool delivered(false);

    // TODO: Return a pointer so we can handle missing pages gracefully.
    // Get the entry for the addressed framework.
    Detail::Entry &entry(Detail::StaticDirectory<Framework>::GetEntry(address.GetFramework()));

    // Pin the entry and lookup the framework registered at the index.
    entry.Lock();
    entry.Pin();
    Framework *const framework(static_cast<Framework *>(entry.GetEntity()));
    entry.Unlock();

    // If a framework is registered at this index then forward the message to it.
    if (framework)
    {
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


