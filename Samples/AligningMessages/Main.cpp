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
// Specifying the alignment requirements of a message type is not enough,
// by itself, to guarantee correct alignment of message allocations. Users
// must also ensure that the allocator used by Theron supports alignment.
// The default allocator, DefaultAllocator, supports alignment and returns
// correctly aligned allocations. Users replacing the default allocator with
// a custom allocator, implementing the \ref IAllocator interface and enabled
// with \ref Theron::AllocatorManager::SetAllocator, supports aligned allocations.
//


// We need to disable this irritating level-4 warning in Visual C++ builds.
#ifdef _MSC_VER
#pragma warning( disable : 4324 )
#endif // _MSC_VER


#include <stdio.h>

#include <Theron/Actor.h>
#include <Theron/Align.h>
#include <Theron/Defines.h>
#include <Theron/AllocatorManager.h>
#include <Theron/Framework.h>
#include <Theron/Receiver.h>

#include <Common/LinearAllocator.h>


namespace Example
{


// A message type that requires alignment.
struct THERON_PREALIGN(128) AlignedMessage
{
    AlignedMessage(const int value) : mValue(value) { }
    int mValue;
    
} THERON_POSTALIGN(128);


// A simple actor that prints the addresses of messages it receives.
class SimpleActor : public Theron::Actor
{
public:

    inline SimpleActor()
    {
        RegisterHandler(this, &SimpleActor::Handler);
    }
    
private:

    inline void Handler(const AlignedMessage &message, const Theron::Address from)
    {
        printf("Address of message in actor: 0x%p\n", &message);
        Send(message, from);
    }
};


} // namespace Example


// Notify Theron of the alignment requirements of the message. This causes
// Theron to request memory of the correct alignment when allocating
// instances of the message. Here we request 128-byte alignment, just as
// a test. On some platforms this is the size of a cache line.
// 
THERON_ALIGN_MESSAGE(Example::AlignedMessage, 128);


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

    Theron::Framework framework;
    Theron::Receiver receiver;
    
    // Instantiate the actor.
    Theron::ActorRef actor(framework.CreateActor<Example::SimpleActor>());

    // Instantiate a message and check its alignment.
    Example::AlignedMessage message(5);
    printf("Address of message in client: 0x%p\n", &message);

    // Send message to the actor.
    actor.Push(message, receiver.GetAddress());

    // Wait for confirmation.
    receiver.Wait();
    return 0;
}

