// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#include <Theron/Detail/Core/ActorCore.h>
#include <Theron/Detail/Core/ActorDestroyer.h>
#include <Theron/Detail/Debug/Assert.h>
#include <Theron/Detail/Directory/ActorDirectory.h>
#include <Theron/Detail/Directory/Directory.h>
#include <Theron/Detail/Threading/Lock.h>

#include <Theron/Actor.h>
#include <Theron/AllocatorManager.h>


namespace Theron
{
namespace Detail
{


void ActorDestroyer::DestroyActor(ActorCore *const actorCore)
{
    // Get a pointer to the actor itself and take its address, before we delete it.
    void *const owner(actorCore->GetOwner());
    Actor *const actor(actorCore->GetParent());
    const Address address(actor->GetAddress());

    {
        Lock lock(Directory::GetMutex());

        // Deregister the actor core first so it can't be sent any messages.
        // We can't actually destroy the core yet, in case the actor does something
        // in its destructor that depends on the core, for example sends a message or
        // even creates another actor.
        if (!ActorDirectory::Instance().DeregisterActor(address))
        {
            // Failed to deregister actor core.
            THERON_FAIL();
        }
    }

    // This seems to actually call the destructor of the derived actor class, as we want.
    actor->~Actor();

    // When freeing the memory we need to use the address of the owning object.
    // This is different in cases where Theron::Actor isn't the first baseclass.
    AllocatorManager::Instance().GetAllocator()->Free(owner);

    {
        Lock lock(Directory::GetMutex());

        // Destroy the actor core and deregister it from its registered address.
        // We have to destroy the core after the actor, and not before, in case
        // the actor does something that depends on the core in its destructor,
        // for example if it sends a message or even creates another actor.
        if (!ActorDirectory::Instance().DestroyActor(address))
        {
            // Failed to destroy actor core.
            THERON_FAIL();
        }
    }
}


} // namespace Detail
} // namespace Theron

