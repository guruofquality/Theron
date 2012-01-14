// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_DETAIL_CORE_ACTORCREATOR_H
#define THERON_DETAIL_CORE_ACTORCREATOR_H


#include <Theron/Detail/BasicTypes.h>
#include <Theron/Detail/Core/ActorAlignment.h>
#include <Theron/Detail/Core/ActorCore.h>
#include <Theron/Detail/Debug/Assert.h>
#include <Theron/Detail/Directory/ActorDirectory.h>
#include <Theron/Detail/Directory/Directory.h>
#include <Theron/Detail/Threading/Lock.h>
#include <Theron/Detail/Threading/Mutex.h>

#include <Theron/Address.h>
#include <Theron/AllocatorManager.h>
#include <Theron/Defines.h>


namespace Theron
{


class Framework;


namespace Detail
{


/// Helper class that creates actors.
/// When an actor is constructed, the Actor constructor fetches
/// a pointer to its referenced actor core from this registry, where
/// it has been set by the framework prior to construction. This hack
/// allows us to pass actor core pointers to actor constructors without
/// actually using a constructor parameter.
class ActorCreator
{
public:

    friend class Theron::Framework;

    /// Gets the remembered actor address.
    inline static Address GetAddress();

    /// Gets the remembered actor core pointer.
    inline static ActorCore *GetCoreAddress();

private:

    ActorCreator();
    ActorCreator(const ActorCreator &other);
    ActorCreator &operator=(const ActorCreator &other);

    /// Creates an instance of an actor type via a provided constructor object.
    /// \note This method can only be called by the Framework.
    template <class ConstructorType>
    inline static typename ConstructorType::ActorType *CreateActor(
        const ConstructorType &constructor,
        Framework *const framework);

    /// Sets the remembered actor address.
    inline static void SetAddress(const Address &address);

    /// Sets the remembered actor core pointer.
    inline static void SetCoreAddress(ActorCore *const address);

    static Mutex smMutex;                   ///< Synchronizes access to the singleton instance.
    static Address smAddress;               ///< Remembered actor address.
    static ActorCore *smCoreAddress;        ///< Remembered actor core pointer.
};


THERON_FORCEINLINE void ActorCreator::SetAddress(const Address &address)
{
    smAddress = address;
}


THERON_FORCEINLINE void ActorCreator::SetCoreAddress(ActorCore *const address)
{
    smCoreAddress = address;
}


THERON_FORCEINLINE Address ActorCreator::GetAddress()
{
    return smAddress;
}


THERON_FORCEINLINE ActorCore *ActorCreator::GetCoreAddress()
{
    return smCoreAddress;
}


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
        bool registered(false);

        {
            Lock lock(Directory::GetMutex());
            ActorDirectory &directory(ActorDirectory::Instance());

            // Register and construct the actor core, passing it the framework and referencing actor.
            // This basically only fails if we run out of memory.
            Actor *const tempActor = reinterpret_cast<Actor *>(actorMemory);
            address = directory.RegisterActor(framework, tempActor);

            if (address != Address::Null())
            {
                actorCore = directory.GetActor(address);
                registered = true;
            }
        }

        if (registered)
        {
            ActorType *actor(0);

            {
                // Lock the mutex and set the static actor core address for the actor core
                // constructor to read during construction. This is an awkward workaround for the
                // inability to pass it via the actor constructor parameters, which we don't control.
                Lock lock(smMutex);

                SetAddress(address);
                SetCoreAddress(actorCore);

                // Use the provided constructor helper to construct the actor within the provided
                // memory buffer. The derived actor class constructor may itself send messages or
                // construct other actors.
                actor = constructor(actorMemory);
            }

            return actor;
        }

        allocator->Free(actorMemory);
    }

    return 0;
}


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_CORE_ACTORCREATOR_H

