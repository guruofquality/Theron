// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#include <Theron/Actor.h>
#include <Theron/Assert.h>
#include <Theron/Counters.h>

#include <Theron/Detail/Containers/ThreadSafeQueue.h>
#include <Theron/Detail/Directory/Directory.h>
#include <Theron/Detail/Directory/Entry.h>
#include <Theron/Detail/Handlers/FallbackHandlerCollection.h>
#include <Theron/Detail/Mailboxes/Mailbox.h>
#include <Theron/Detail/MailboxProcessor/ProcessorContext.h>
#include <Theron/Detail/MailboxProcessor/WorkItem.h>
#include <Theron/Detail/Messages/IMessage.h>
#include <Theron/Detail/Messages/MessageCreator.h>


namespace Theron
{
namespace Detail
{


void WorkItem::Process(Mailbox *const mailbox, WorkerThreadStore *const store)
{
    ProcessorContext *const context(&store->mProcessorContext);

    // Load the context data from the worker thread's processor context.
    ThreadSafeQueue<Mailbox> *const workQueue(context->mWorkQueue);
    FallbackHandlerCollection *const fallbackHandlers(context->mFallbackHandlers);
    IAllocator *const messageAllocator(context->mMessageAllocator);

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


} // namespace Detail
} // namespace Theron


