// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#include <Theron/Actor.h>
#include <Theron/Assert.h>
#include <Theron/Counters.h>
#include <Theron/Defines.h>

#include <Theron/Detail/Directory/Directory.h>
#include <Theron/Detail/Directory/Entry.h>
#include <Theron/Detail/Handlers/FallbackHandlerCollection.h>
#include <Theron/Detail/Mailboxes/Mailbox.h>
#include <Theron/Detail/MailboxProcessor/Processor.h>
#include <Theron/Detail/MailboxProcessor/WorkQueue.h>
#include <Theron/Detail/Messages/IMessage.h>
#include <Theron/Detail/Messages/MessageCreator.h>
#include <Theron/Detail/Threading/Utils.h>


namespace Theron
{
namespace Detail
{


void Processor::ProcessMailbox(Context *const context, Mailbox *const mailbox)
{
    // Load the context data from the worker thread's processor context.
    // Actors that need more processing are always pushed onto the worker thread's local queue.
    WorkQueue *const workQueue(context->mLocalWorkQueue);
    FallbackHandlerCollection *const fallbackHandlers(context->mFallbackHandlers);
    IAllocator *const messageAllocator(&context->mMessageCache);

    THERON_ASSERT(workQueue);
    THERON_ASSERT(fallbackHandlers);
    THERON_ASSERT(messageAllocator);

    // Increment the context's message processing event counter.
    context->mCounters[Theron::COUNTER_MESSAGES_PROCESSED].Increment();

    // Pin the mailbox and get the registered actor and the first queued message.
    // At this point the mailbox shouldn't be enqueued in any other work items,
    // even if it contains more than one unprocessed message. This ensures that
    // each mailbox is only processed by one worker thread at a time.
    mailbox->Lock();
    mailbox->Pin();
    Actor *const actor(mailbox->GetActor());
    IMessage *const message(mailbox->Front());
    mailbox->Unlock();

    // If an entity is registered at the mailbox then process it.
    if (actor)
    {
        // Store a pointer to the context data for this thread in the actor.
        // We'll need it to send messages if any of the registered handlers
        // call Actor::Send, but we can't pass it through from here because
        // the handlers are user code.
        THERON_ASSERT(actor->mProcessorContext == 0);
        actor->mProcessorContext = context;

        actor->ProcessMessage(fallbackHandlers, message);

        // Zero the context pointer, in case it's next accessed by a non-worker thread.
        THERON_ASSERT(actor->mProcessorContext == context);
        actor->mProcessorContext = 0;
    }

    // Unpin the mailbox, allowing the registered actor to be changed by other threads.
    mailbox->Lock();
    mailbox->Unpin();
    mailbox->Unlock();

    if (actor == 0 && fallbackHandlers)
    {
        fallbackHandlers->Handle(message);
    }

    // Pop the message we just processed from the mailbox, then check whether the
    // mailbox is now empty, and reschedule the mailbox if it's not.
    // The locking of the mailbox here and in the main scheduling ensures that
    // mailboxes are always enqueued if they unprocessed messages, but at most once
    // at any time.
    mailbox->Lock();

    mailbox->Pop();
    if (!mailbox->Empty())
    {
        workQueue->Push(mailbox);
    }

    mailbox->Unlock();

    // Destroy the message, but only after we've popped it from the queue.
    MessageCreator::Destroy(messageAllocator, message);
}


void Processor::YieldPolite(uint32_t &counter)
{
    // This yield strategy scales from a simple 'nop' to putting the calling thread to sleep.
    // This implementation is based roughly on http://www.1024cores.net/home/lock-free-algorithms/tricks/spinning
    if (++counter < 10)
    {
        Utils::YieldToHyperthread();
    }
    else if (counter < 20)
    {
        for (uint32_t i = 0; i < 50; ++i)
        {
            Utils::YieldToHyperthread();
        }
    }
    else if (counter < 22)
    {
        Utils::YieldToLocalThread();
    }
    else if (counter < 24)
    {
        Utils::YieldToAnyThread();
    }
    else
    {
        Utils::SleepThread(1UL);
    }
}


void Processor::YieldStrong(uint32_t &counter)
{
    // This 'strong' implementation yields after spinning for a while, but never sleeps.
    if (++counter < 10)
    {
        Utils::YieldToHyperthread();
    }
    else if (counter < 20)
    {
        for (uint32_t i = 0; i < 50; ++i)
        {
            Utils::YieldToHyperthread();
        }
    }
    else if (counter < 22)
    {
        Utils::YieldToLocalThread();
    }
    else
    {
        Utils::YieldToAnyThread();
    }
}


void Processor::YieldAggressive(uint32_t &counter)
{
    // This 'aggressive' implementation never yields or sleeps.
    // It does however pause to allow another thread running on the same hyperthreaded core to proceed.
    if (++counter < 10)
    {
        Utils::YieldToHyperthread();
    }
    else if (counter < 20)
    {
        for (uint32_t i = 0; i < 50; ++i)
        {
            Utils::YieldToHyperthread();
        }
    }
    else if (counter < 22)
    {
        for (uint32_t i = 0; i < 100; ++i)
        {
            Utils::YieldToHyperthread();
        }
    }
    else
    {
        for (uint32_t i = 0; i < 200; ++i)
        {
            Utils::YieldToHyperthread();
        }
    }
}


} // namespace Detail
} // namespace Theron


