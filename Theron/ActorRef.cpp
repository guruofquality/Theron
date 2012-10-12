// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#include <Theron/Actor.h>
#include <Theron/ActorRef.h>
#include <Theron/AllocatorManager.h>
#include <Theron/Framework.h>

#include <Theron/Detail/Threading/Atomic.h>


namespace Theron
{


Address ActorRef::GetAddress() const
{
    THERON_ASSERT(mActor);
    return mActor->GetAddress();
}


void ActorRef::Reference()
{
    if (mActor)
    {
        mActor->Reference();
    }
}


void ActorRef::Dereference()
{
    if (mActor)
    {
        if (mActor->Dereference())
        {
            // There's no longer any garbage collection in Theron so we destroy the actor immediately.
            // We need to explicitly destruct the actor because, in this legacy usage pattern, we
            // constructed the derived actor with placement new in Framework::CreateActor.
            // We call the baseclass destructor and it calls the derived class destructor too.
            // I'm not totally clear on why this works but I guess it's because of the virtual destructors.
            mActor->~Actor();

            // Since the Actor baseclass may be located at some offset within the final derived
            // actor type, the address of the final actor may not match the address of the baseclass.
            // So when freeing the memory block in which the actor was allocated we need to remember
            // and explicitly use the address of the actual memory block.
            AllocatorManager::Instance().GetAllocator()->Free(mActor->mMemory);
            mActor = 0;
        }
    }
}


uint32_t ActorRef::GetNumQueuedMessages() const
{
    return mActor->GetNumQueuedMessages();
}


Detail::Processor::Context &ActorRef::GetProcessorContext()
{
    return mActor->GetFramework().mProcessorContext;
}


uint32_t ActorRef::GetFrameworkIndex() const
{
    return mActor->GetFramework().GetIndex();
}


} // namespace Theron
