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
    Actor *const actor(actorCore->GetParent());
    const Address address(actor->GetAddress());

    // Lock the directory to make sure no one can send the actor a message.
    Lock lock(Directory::GetMutex());

    // This seems to actually call the derived actor class destructor, as we want.
    actor->~Actor();
    AllocatorManager::Instance().GetAllocator()->Free(actor);

    // Destroy the actor core and deregister it from its registered address.
    // We have to destroy the core after the actor, and not before, in case
    // the actor does something that depends on the core in its destructor,
    // for example if it sends a message or even creates another actor.
    if (!ActorDirectory::Instance().DeregisterActor(address))
    {
        // Failed to deregister actor core.
        THERON_FAIL();
    }
}


} // namespace Detail
} // namespace Theron

