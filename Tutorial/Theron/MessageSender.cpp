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


bool MessageSender::DeliverByName(
    EndPoint *const endPoint,
    ProcessorContext *const processorContext,
    const uint32_t localFrameworkIndex,
    IMessage *const message,
    const String &name)
{
    THERON_ASSERT_MSG(endPoint, "Sending messages addressed by name requires a Theron::EndPoint");

    // Search the local endPoint for a mailbox with the given name.
    // If there is a local match we fall through using the retrieved index, and we don't
    // bother to push the message out onto the network, since names are globally unique.
    Index index;
    if (endPoint->Lookup(name, index))
    {
        // Non-inlined call.
        const Address newAddress(name, index);
        return DeliverByIndex(
            processorContext,
            localFrameworkIndex,
            message,
            newAddress);
    }

    // If there isn't a local match we send the message out onto the network.
    return endPoint->RequestSend(message, name);
}


bool MessageSender::DeliverByIndex(
    ProcessorContext *const processorContext,
    const uint32_t localFrameworkIndex,
    IMessage *const message,
    const Address &address)
{
    const Index index(address.mIndex);
    THERON_ASSERT(index.mUInt32);
    
    bool delivered(false);

    // Which framework is the addressed entity in?
    const uint32_t targetFrameworkIndex(index.mComponents.mFramework);
    if (targetFrameworkIndex == localFrameworkIndex)
    {
        // Is the message addressed to an actor in this framework?
        delivered = DeliverToActorInThisFramework(
            processorContext,
            message,
            address);
    }
    else
    {
        delivered = DeliverToLocalMailbox(message, address);
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


bool MessageSender::DeliverToActorInLocalFramework(
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


