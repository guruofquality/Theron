// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


//
// This benchmark is a simple test of the memory overhead of Actor objects within Theron.
// Because user actor classes must derive from the Theron::Actor baseclass to be
// valid actors, they inherit some internal state provided by it.
// This introduces some size overhead into actors even before any user state has
// been added. This demo shows this by defining a trivial "empty" actor with no
// user state at all, and reporting its size. Note that the reported size includes
// the core implementation of the actor, but not memory cached in local memory block
// caches, which are not considered part of the actor, but will in general also incurr
// a per-actor memory overhead.
//


#include <stdio.h>

#include <Theron/Actor.h>


namespace Example
{


// A simple "empty" actor with no user state at all.
class EmptyActor : public Theron::Actor
{
};


} // namespace Example


int main()
{
    printf("The 'empty' actor has size %d bytes\n", static_cast<int>(sizeof(Example::EmptyActor)));
    printf("Additionally, the core object referenced by the actor has size %d bytes\n", static_cast<int>(sizeof(Theron::Detail::ActorCore)));
    return 0;
}

