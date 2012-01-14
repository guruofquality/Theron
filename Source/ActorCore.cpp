// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#include <Theron/Detail/BasicTypes.h>
#include <Theron/Detail/Core/ActorCore.h>
#include <Theron/Detail/Debug/Assert.h>
#include <Theron/Detail/Directory/Directory.h>
#include <Theron/Detail/Messages/MessageCreator.h>
#include <Theron/Detail/Threading/Lock.h>

#include <Theron/Actor.h>
#include <Theron/AllocatorManager.h>
#include <Theron/Framework.h>


namespace Theron
{
namespace Detail
{


ActorCore::ActorCore() :
  mNext(0),
  mParent(0),
  mFramework(0),
  mSequence(0),
  mMessageCount(0),
  mMessageQueue(),
  mMessageHandlers(),
  mState(0)
{
    // Actor cores shouldn't be default-constructed.
    THERON_FAIL();
}


ActorCore::ActorCore(const uint32_t sequence, Framework *const framework, Actor *const actor) :
  mNext(0),
  mParent(actor),
  mFramework(framework),
  mSequence(sequence),
  mMessageCount(0),
  mMessageQueue(),
  mMessageHandlers(),
  mState(STATE_REFERENCED)
{
    THERON_ASSERT(GetSequence() != 0);
    THERON_ASSERT(mFramework != 0);
}


ActorCore::~ActorCore()
{
    // We don't need to lock this because only one thread can access it at a time.
    // Free all currently allocated handler objects.
    while (IMessageHandler *const handler = mMessageHandlers.Front())
    {
        mMessageHandlers.Remove(handler);
        AllocatorManager::Instance().GetAllocator()->Free(handler);
    }

    {
        // The directory lock is used to protect the global free list.
        Lock directoryLock(Directory::GetMutex());
        Lock frameworkLock(mFramework->GetMutex());

        // Free any left-over messages that haven't been processed.
        // This is undesirable but can happen if the actor is killed while
        // still processing messages.
        while (IMessage *const message = mMessageQueue.Pop())
        {
            MessageCreator::Destroy(message);
            --mMessageCount;
        }

        THERON_ASSERT(mMessageCount == 0);
    }
}


Mutex &ActorCore::GetMutex() const
{
    // We reuse the main message loop mutex to avoid locking an additional mutex.
    return mFramework->GetMutex();
}


void ActorCore::UpdateHandlers()
{
    MessageHandlerList &newHandlerList(mParent->GetNewHandlerList());

    // Add any new handlers. We do this first in case any are already marked for deletion.
    // The handler class contains the next pointer, so handlers can only be in one list at a time.
    while (IMessageHandler *const handler = newHandlerList.Front())
    {
        newHandlerList.Remove(handler);
        mMessageHandlers.Insert(handler);
    }

    // Transfer all handlers to the new handler list, omitting any which are marked for deletion.    
    while (IMessageHandler *const handler = mMessageHandlers.Front())
    {
        mMessageHandlers.Remove(handler);
        if (handler->IsMarked())
        {
            AllocatorManager::Instance().GetAllocator()->Free(handler);
        }
        else
        {
            newHandlerList.Insert(handler);
        }
    }

    // Finally transfer the filtered handlers back in the actual list.
    while (IMessageHandler *const handler = newHandlerList.Front())
    {
        newHandlerList.Remove(handler);
        mMessageHandlers.Insert(handler);
    }
}


bool ActorCore::ExecuteDefaultHandler(IMessage *const message)
{
    IDefaultHandler *const defaultHandler = mParent->GetDefaultHandler();
    if (defaultHandler)
    {
        defaultHandler->Handle(mParent, message);
        return true;
    }

    return false;
}


bool ActorCore::ExecuteFallbackHandler(IMessage *const message)
{
    return mFramework->ExecuteFallbackHandler(message);
}


void ActorCore::Unreference()
{
    // Schedule the actor core to make the threadpool garbage collect it.
    mState &= ~STATE_REFERENCED;
    mFramework->Schedule(this);
}


} // namespace Detail
} // namespace Theron


