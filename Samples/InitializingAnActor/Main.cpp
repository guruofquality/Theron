// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


//
// This sample shows how to initialize an actor.
//


#include <stdio.h>

#include <Theron/Framework.h>
#include <Theron/Receiver.h>
#include <Theron/Actor.h>


// A simple actor that can be initialized explicitly on creation.
class InitializedActor : public Theron::Actor
{
public:

    // Actors can expose Parameters member structs that allow
    // callers to pass parameter values to the actor on creation.
    // If provided, the parameters class must be called Parameters.
    // It can be a class or struct and contain any initialization data
    // required.
    struct Parameters
    {
        int mValue;
    };

    // Explicit constructor. Optional. Allows the actor to be created
    // with an explicit Parameters object. How the parameters object is
    // interpreted is up to the implementation.
    inline explicit InitializedActor(const Parameters &params) : mValue(params.mValue)
    {
        printf("InitializedActor explicitly constructed with parameter value %d\n", params.mValue);
    }

private:

    // Private member data owned by the actor.
    unsigned int mValue;
};


int main()
{
    Theron::Framework framework;

    // Create an instance of the actor using explicit parameters.
    InitializedActor::Parameters params;
    params.mValue = 1;

    Theron::ActorRef actor(framework.CreateActor<InitializedActor>(params));
    printf("Created actor has address %d\n", actor.GetAddress().AsInteger());

    return 0;
}

