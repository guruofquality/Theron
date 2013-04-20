// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_DETAIL_LEGACY_ACTORREGISTRY_H
#define THERON_DETAIL_LEGACY_ACTORREGISTRY_H


#include <Theron/Assert.h>
#include <Theron/BasicTypes.h>
#include <Theron/Defines.h>

#include <Theron/Detail/Threading/SpinLock.h>


namespace Theron
{


class Actor;
class Framework;


namespace Detail
{


/**
Static helper that stores a map of Actor pointers to framework pointers.
Used on creation of actors using the deprecated 3.x CreateActor actor creation pattern.
*/
class ActorRegistry
{
public:

    struct Entry
    {
        Actor *mActor;
        Framework *mFramework;
        void *mMemory;
        Entry *mNext;
    };

    /**
    Registers a user-allocated entry mapping an Actor pointer to a Framework pointer.
    */
    static void Register(Entry *const entry);

    /**
    Deregisters a previously registered entry.
    */
    static void Deregister(Entry *const entry);

    /**
    Finds an entry previously registered for the given actor.
    */
    static Entry *Lookup(Actor *const actor);

private:

    ActorRegistry();
    ActorRegistry(const ActorRegistry &other);
    ActorRegistry &operator=(const ActorRegistry &other);

    static SpinLock smSpinLock;     ///< Protects access to a static list of registered entries.
    static Entry *smHead;           ///< Head of a static list of registered entries.
};


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_LEGACY_ACTORREGISTRY_H
