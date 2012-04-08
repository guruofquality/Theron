// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_DETAIL_DIRECTORY_ACTORDIRECTORY_H
#define THERON_DETAIL_DIRECTORY_ACTORDIRECTORY_H


#include <Theron/Detail/BasicTypes.h>
#include <Theron/Detail/Core/ActorCore.h>
#include <Theron/Detail/Debug/Assert.h>
#include <Theron/Detail/PagedPool/PagedPool.h>

#include <Theron/Address.h>
#include <Theron/Defines.h>


namespace Theron
{


class Framework;
class Actor;


namespace Detail
{


/// A directory mapping unique addresses to actors.
/// The directory is also a pool in which actor cores are allocated.
class ActorDirectory
{
public:

    /// Gets a reference to the single directory object.
    inline static ActorDirectory &Instance();

    /// Default constructor.
    inline ActorDirectory();

    /// Returns the number of actors currently registered.
    inline uint32_t Count() const;

    /// Registers an actor and returns its unique address.
    Address RegisterActor(Framework *const framework, void *const owner, Actor *const actor);

    /// Deregisters the actor at the given address, preventing it from receiving messages.
    /// \note The address can be the address of a currently registered actor.
    bool DeregisterActor(const Address &address);

    /// Destroys the actor at the given address.
    /// \note The address can be the address of a currently registered actor.
    bool DestroyActor(const Address &address);

    /// Gets a pointer to the actor core at the given address.
    /// \note The address can be the address of a currently registered actor.
    inline ActorCore *GetActor(const Address &address) const;

private:

    typedef PagedPool<ActorCore, THERON_MAX_ACTORS> ActorPool;

    ActorDirectory(const ActorDirectory &other);
    ActorDirectory &operator=(const ActorDirectory &other);

    static ActorDirectory smInstance;       ///< Single, static instance of the class.

    ActorPool mActorPool;                   ///< Pool of system-allocated actor cores.
};


THERON_FORCEINLINE ActorDirectory &ActorDirectory::Instance()
{
    return smInstance;
}


THERON_FORCEINLINE ActorDirectory::ActorDirectory() : mActorPool()
{
}


THERON_FORCEINLINE uint32_t ActorDirectory::Count() const
{
    return mActorPool.Count();
}


THERON_FORCEINLINE ActorCore *ActorDirectory::GetActor(const Address &address) const
{
    THERON_ASSERT(Address::IsActorAddress(address));
    const uint32_t index(address.GetIndex());

    // The entry in the actor pool is memory for an ActorCore object.
    void *const entry(mActorPool.GetEntry(index));
    if (entry)
    {
        // Reject the actor core if its sequence number doesn't match.
        // This guards against new actor cores constructed at the same indices as old ones.
        ActorCore *const actorCore(reinterpret_cast<ActorCore *>(entry));
        if (actorCore->GetSequence() == address.GetSequence())
        {
            return actorCore;
        }
    }

    return 0;
}


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_DIRECTORY_ACTORDIRECTORY_H

