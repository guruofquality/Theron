// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#include <new>

#include <Theron/Detail/Directory/ActorDirectory.h>
#include <Theron/Detail/Debug/Assert.h>

#include <Theron/Actor.h>
#include <Theron/Framework.h>


namespace Theron
{
namespace Detail
{


ActorDirectory ActorDirectory::smInstance;


Address ActorDirectory::RegisterActor(Framework *const framework, void *const owner, Actor *const actor)
{
    uint32_t index(0);
    if (mActorPool.Allocate(index))
    {
        const Address address(Address::MakeActorAddress(index));

        // The entry in the actor pool is memory for an ActorCore object.
        void *const entry(mActorPool.GetEntry(index));
        THERON_ASSERT(entry);

        // Construct the actor core, initializing its sequence number.
        // The sequence number is used to authenticate it later.
        new (entry) ActorCore(address.GetSequence(), framework, owner, actor);

        return address;
    }

    return Address::Null();
}


bool ActorDirectory::DeregisterActor(const Address &address)
{
    THERON_ASSERT(Address::IsActorAddress(address));
    const uint32_t index(address.GetIndex());

    // The entry in the actor pool is memory for an ActorCore object.
    void *const entry(mActorPool.GetEntry(index));
    if (entry)
    {
        // Reject the actor core if its address (ie. sequence number) doesn't match.
        // This guards against new actor cores constructed at the same indices as old ones.
        ActorCore *const actorCore(reinterpret_cast<ActorCore *>(entry));
        if (actorCore->GetSequence() == address.GetSequence())
        {
            // Null the sequence number of the actor core so it doesn't match any future queries.
            // Zero is an invalid sequence number which isn't assigned to any actor.
            actorCore->SetSequence(0);
            return true;
        }
    }

    return false;
}


bool ActorDirectory::DestroyActor(const Address &address)
{
    THERON_ASSERT(Address::IsActorAddress(address));
    const uint32_t index(address.GetIndex());

    // The entry in the actor pool is memory for an ActorCore object.
    void *const entry(mActorPool.GetEntry(index));
    if (entry)
    {
        // Reject the actor core if its address (ie. sequence number) doesn't match.
        // This guards against new actor cores constructed at the same indices as old ones.
        ActorCore *const actorCore(reinterpret_cast<ActorCore *>(entry));

        // Destruct the actor core manually since it's buffer-allocated so we can't call delete.
        actorCore->~ActorCore();
        if (mActorPool.Free(index))
        {
            return true;
        }
    }

    return false;
}


} // namespace Detail
} // namespace Theron


