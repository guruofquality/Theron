// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


//
// This sample shows how to use Theron with actor implementations that have
// specialized memory alignment requirements.
//
// The required memory alignment of a user-written actor class can be specified
// using the THERON_ALIGN_ACTOR macro defined in Theron/Align.h.
//
// Use of this macro is optional. Using it, users can notify Theron of any
// specialized memory alignment requirements of their custom actor classes.
// If the memory alignment of an actor type is specified using this macro,
// Theron will request correctly aligned memory when allocating instances
// of that actor type in \ref Framework::CreateActor. If not, then a default
// alignment of four bytes will be used.
//
// Specifying the alignment requirements of an actor type is not enough,
// by itself, to guarantee correct alignment of actor allocations. Users must
// also ensure that the allocator used by Theron supports alignment.
// The default allocator, DefaultAllocator, supports alignment and returns
// correctly aligned allocations. Users replacing the default allocator with
// a custom allocator, implementing the \ref IAllocator interface and enabled
// with \ref Theron::AllocatorManager::SetAllocator, supports aligned allocations.
//


#include <stdio.h>

#include <Theron/Actor.h>
#include <Theron/Align.h>
#include <Theron/AllocatorManager.h>
#include <Theron/Framework.h>

#include <Common/LinearAllocator.h>


namespace Example
{


// A simple actor that requires aligned memory allocation.
class AlignedActor : public Theron::Actor
{
public:

    inline AlignedActor()
    {
        printf("AlignedActor instantiated at memory address 0x%p\n", this);
    }
};


} // namespace Example


// Notify Theron of the alignment requirements of the actor class. This causes
// Theron to request memory of the correct alignment when allocating
// instances of the actor class. Here we request 128-byte alignment, just as
// a test. On some platforms this is the size of a cache line.
// 
THERON_ALIGN_ACTOR(Example::AlignedActor, 128);


int main()
{
    // Construct a LinearAllocator around a memory buffer. The DefaultAllocator used
    // by Theron by default supports alignment, so can be safely used. Here we show
    // the use of a custom allocator, which must also support alignment, as an example.
    const unsigned int BUFFER_SIZE(16384); 
    unsigned char buffer[BUFFER_SIZE];
    Example::LinearAllocator linearAllocator(buffer, BUFFER_SIZE);

    // Set the custom allocator.
    Theron::AllocatorManager::Instance().SetAllocator(&linearAllocator);

    // Instantiate the actor so it prints out its alignment.
    Theron::Framework framework;
    Theron::ActorRef actor(framework.CreateActor<Example::AlignedActor>());

    printf("Finished\n");
    return 0;
}

