// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_DETAIL_CORE_ACTORCREATOR_H
#define THERON_DETAIL_CORE_ACTORCREATOR_H


#include <Theron/BasicTypes.h>
#include <Theron/Address.h>
#include <Theron/AllocatorManager.h>
#include <Theron/Defines.h>

#include <Theron/Detail/Core/ActorAlignment.h>
#include <Theron/Detail/Core/ActorCore.h>
#include <Theron/Detail/Debug/Assert.h>
#include <Theron/Detail/Directory/ActorDirectory.h>
#include <Theron/Detail/Directory/Directory.h>
#include <Theron/Detail/Threading/Mutex.h>
#include <Theron/Detail/Threading/Lock.h>


namespace Theron
{


class Framework;


namespace Detail
{


/**
Static helper class that creates actors. This is a detail class and is not for general use.
*/
class ActorCreator
{
public:

    struct Entry
    {
        void *mLocation;
        Address mAddress;
        ActorCore *mActorCore;
        Entry *mNext;
    };

    /// Creates an instance of an actor type via a provided constructor object.
    /// \note This method is not for public use and should only be called by the Framework.
    template <class ConstructorType>
    inline static typename ConstructorType::ActorType *CreateActor(
        const ConstructorType &constructor,
        Framework *const framework);

    /// Get the entry holding member data for the actor being constructed at the given address.
    static Entry *Get(void *const location);

private:

    ActorCreator();
    ActorCreator(const ActorCreator &other);
    ActorCreator &operator=(const ActorCreator &other);

    /// Register an entry holding an address and member data for an actor being constructed.
    static void Register(Entry *const entry);

    /// Deregister a previously registered entry.
    static void Deregister(Entry *const entry);

    static Mutex smMutex;           ///< Protects access to a static list of registered entries.
    static Entry *smHead;           ///< Head of a static list of registered entries.
};


template <class ConstructorType>
THERON_FORCEINLINE typename ConstructorType::ActorType *ActorCreator::CreateActor(
    const ConstructorType &constructor,
    Framework *const framework)
{
    typedef typename ConstructorType::ActorType ActorType;

    IAllocator *const allocator(AllocatorManager::Instance().GetAllocator());
    Address address(Address::Null());
    ActorCore *actorCore(0);

    // Allocate a separate, aligned memory block for the actor itself.
    const uint32_t size(sizeof(ActorType));
    const uint32_t alignment(ActorAlignment<ActorType>::ALIGNMENT);
    
    void *const actorMemory = allocator->AllocateAligned(size, alignment);
    if (actorMemory)
    {
        // We reinterpret cast so we can access the memory layout before construction.
        ActorType *actor(reinterpret_cast<ActorType *>(actorMemory));

        {
            Detail::Lock lock(Directory::GetMutex());
            ActorDirectory &directory(ActorDirectory::Instance());

            // Register and construct the actor core, passing it the framework and referencing actor.
            // Note we have to pass the pointer to the Actor baseclass itself, which may not be
            // the same if the Actor baseclass isn't first in the list of baseclasses.
            // This basically only fails if we run out of memory.
            Actor *const baseclass = static_cast<Actor *>(actor);
            address = directory.RegisterActor(framework, actor, baseclass);

            if (address != Address::Null())
            {
                actorCore = directory.GetActor(address);
            }
        }

        // Was the registration of the actor successful?
        if (actorCore)
        {
            // We register the data for the Actor baseclass in a static dictionary, and the
            // Actor baseclass constructor reads it from there. We do it this hacky way because we
            // don't 'own' the constructor of the derived actor class, so can't pass the data
            // through it to the constructor of the baseclass, as we would like.
            Entry entry;
            entry.mLocation = static_cast<Actor *>(actor);
            entry.mAddress = address;
            entry.mActorCore = actorCore;

            Register(&entry);

            // Use the provided constructor helper to construct the actor within the provided
            // memory buffer. The derived actor class constructor may itself send messages or
            // construct other actors.
            constructor(actorMemory);

            Deregister(&entry);

            return actor;
        }

        allocator->Free(actorMemory);
    }

    return 0;
}


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_CORE_ACTORCREATOR_H

