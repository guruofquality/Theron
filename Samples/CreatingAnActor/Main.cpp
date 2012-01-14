// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


//
// This sample shows how to create an actor.
//


#include <stdio.h>

#include <Theron/Framework.h>
#include <Theron/Actor.h>


// A trivial actor that does nothing.
class TrivialActor : public Theron::Actor
{
public:

    inline TrivialActor()
    {
        printf("TrivialActor constructed\n");
    }
};


int main()
{
    // Create an instance of the framework.
    // This is the main coordinator that hosts and manages actors.
    Theron::Framework framework;

    // Create a single instance of our trivial actor.
    // The actor is allocated by the framework using a default allocator
    // which just uses global new(). We get back a reference to the actor,
    // rather than the actor itself.
    Theron::ActorRef simpleActor(framework.CreateActor<TrivialActor>());

    // When we're finished with the actor instance we can just let the
    // reference we hold to it go out of scope and be destructed.
    // Actors are reference-counted and automatically destroyed when
    // they become unreferenced.

    return 0;
}

