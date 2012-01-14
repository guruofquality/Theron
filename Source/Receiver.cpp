// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#include <Theron/Detail/Debug/Assert.h>
#include <Theron/Detail/Directory/Directory.h>
#include <Theron/Detail/Directory/ReceiverDirectory.h>
#include <Theron/Detail/MessageCache/MessageCache.h>
#include <Theron/Detail/Messages/MessageCreator.h>
#include <Theron/Detail/Threading/Lock.h>

#include <Theron/Address.h>
#include <Theron/AllocatorManager.h>
#include <Theron/Defines.h>
#include <Theron/IAllocator.h>
#include <Theron/Receiver.h>


namespace Theron
{


Receiver::Receiver() :
  mAddress(Address::Null()),
  mMessageHandlers(),
  mMonitor(),
  mMessagesReceived(0)
{
    // Reference the global free list to ensure it's created.
    Detail::MessageCache::Instance().Reference();

    {
        Detail::Lock lock(Detail::Directory::GetMutex());

        // Register this receiver with the directory and get its unique address.
        mAddress = Detail::ReceiverDirectory::Instance().RegisterReceiver(this);
        THERON_ASSERT(mAddress != Address::Null());
    }
}


Receiver::~Receiver()
{
    {
        Detail::Lock lock(Detail::Directory::GetMutex());
        Detail::ReceiverDirectory::Instance().DeregisterReceiver(GetAddress());
    }

    {
        Detail::Lock lock(mMonitor.GetMutex());

        // Free all currently allocated handler objects.
        while (Detail::IReceiverHandler *const handler = mMessageHandlers.Front())
        {
            mMessageHandlers.Remove(handler);
            AllocatorManager::Instance().GetAllocator()->Free(handler);
        }
    }

    // Dereference the global free list to ensure it's destroyed.
    Detail::MessageCache::Instance().Dereference();
}


void Receiver::Push(Detail::IMessage *const message)
{
    THERON_ASSERT(message);

    {
        Detail::Lock lock(mMonitor.GetMutex());
    
        MessageHandlerList::Iterator handlers(mMessageHandlers.Begin());
        const MessageHandlerList::Iterator handlersEnd(mMessageHandlers.End());

        while (handlers != handlersEnd)
        {
            // Execute the handler.
            // It does nothing if it can't handle the message type.
            Detail::IReceiverHandler *const handler(*handlers);
            handler->Handle(message);

            ++handlers;
        }

        // Wake up anyone who's waiting for a message to arrive.
        ++mMessagesReceived;

        mMonitor.Pulse();
    }

    // Free the message, whether it was handled or not.
    // The directory lock is used to protect the global free list.
    Detail::Lock lock(Detail::Directory::GetMutex());
    Detail::MessageCreator::Destroy(message);
}


} // namespace Theron


