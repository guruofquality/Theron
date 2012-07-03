// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#include <Theron/Detail/Core/ActorCore.h>
#include <Theron/Detail/Directory/ActorDirectory.h>
#include <Theron/Detail/Directory/ReceiverDirectory.h>
#include <Theron/Detail/Messages/IMessage.h>
#include <Theron/Detail/Messages/MessageSender.h>
#include <Theron/Detail/Threading/Mutex.h>
#include <Theron/Detail/Threading/Lock.h>

#include <Theron/Address.h>
#include <Theron/Framework.h>


namespace Theron
{
namespace Detail
{


bool MessageSender::Deliver(
    uint32_t *const pulseCount,
    const Framework *const framework,
    IMessage *const message,
    const Address &address)
{
    if (Address::IsActorAddress(address))
    {
        // We use a single mutex to protect access to the work queue and the actor message queues.
        // This turns out to be faster than using separate mutexes, perhaps because of the use of
        // expensive locks rather than lock-free primitives. Given this, our strategy is to reduce
        // the locks in the core actor processing loop down to just one, and instead make the code
        // within the lock as fast as possible.
        Lock frameworkLock(framework->GetMutex());

        ActorCore *const actorCore = ActorDirectory::Instance().GetActor(address);
        if (actorCore)
        {
            // Push the message onto the actor's dedicated message queue.
            actorCore->Push(message);

            // Schedule the actor for processing and wake a worker thread to process it.
            const bool pulsed(framework->Schedule(actorCore));
            const uint32_t pulseCountIncrement(static_cast<uint32_t>(pulsed));

            // We exploit the fact that C++ bools are either 0 or 1.
            THERON_ASSERT(pulseCountIncrement <= 1);
            *pulseCount += pulseCountIncrement;

            return true;
        }
    }
    else
    {
        Receiver *const receiver = ReceiverDirectory::Instance().GetReceiver(address);
        if (receiver)
        {
            receiver->Push(message);
            return true;
        }
    }

    // Call the framework's fallback handler, if one was provided.
    framework->ExecuteFallbackHandler(message);

    return false;
}


bool MessageSender::TailDeliver(
    const Framework *const framework,
    IMessage *const message,
    const Address &address)
{
    if (Address::IsActorAddress(address))
    {
        // We use a single mutex to protect access to the work queue and the actor message queues.
        // This turns out to be faster than using separate mutexes, perhaps because of the use of
        // expensive locks rather than lock-free primitives. Given this, our strategy is to reduce
        // the locks in the core actor processing loop down to just one, and instead make the code
        // within the lock as fast as possible.
        Lock frameworkLock(framework->GetMutex());

        ActorCore *const actorCore = ActorDirectory::Instance().GetActor(address);
        if (actorCore)
        {

            // Push the message onto the actor's dedicated message queue.
            actorCore->Push(message);

            // Schedule the actor for processing without waking a worker thread.
            framework->TailSchedule(actorCore);

            return true;
        }
    }
    else
    {
        Receiver *const receiver = ReceiverDirectory::Instance().GetReceiver(address);
        if (receiver)
        {
            receiver->Push(message);
            return true;
        }
    }

    // Call the framework's fallback handler, if one was provided.
    framework->ExecuteFallbackHandler(message);

    return false;
}


} // namespace Detail
} // namespace Theron

