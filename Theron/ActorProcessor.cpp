// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#include <Theron/Actor.h>
#include <Theron/Assert.h>
#include <Theron/Defines.h>

#include <Theron/Detail/Handlers/FallbackHandlerCollection.h>
#include <Theron/Detail/Scheduler/ActorProcessor.h>
#include <Theron/Detail/Scheduler/MailboxContext.h>


namespace Theron
{
namespace Detail
{


void ActorProcessor::Process(
    WorkerContext *const workerContext,
    Actor *const actor,
    IMessage *const message)
{
    // Load the context data from the worker thread's mailbox context.
    MailboxContext *const context(&workerContext->mMailboxContext);
    FallbackHandlerCollection *const fallbackHandlers(context->mFallbackHandlers);

    THERON_ASSERT(actor);
    THERON_ASSERT(message);

    // Store a pointer to the context data for this thread in the actor.
    // We'll need it to send messages if any of the registered handlers
    // call Actor::Send, but we can't pass it through from here because
    // the handlers are user code.
    THERON_ASSERT(actor->mMailboxContext == 0);
    actor->mMailboxContext = context;

    actor->ProcessMessage(fallbackHandlers, message);

    // Zero the context pointer, in case it's next accessed by a non-worker thread.
    THERON_ASSERT(actor->mMailboxContext == context);
    actor->mMailboxContext = 0;
}


} // namespace Detail
} // namespace Theron


