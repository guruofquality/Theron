// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_DETAIL_THREADPOOL_ACTORPROCESSOR_H
#define THERON_DETAIL_THREADPOOL_ACTORPROCESSOR_H


#include <Theron/Detail/Containers/IntrusiveQueue.h>
#include <Theron/Detail/Core/ActorCore.h>
#include <Theron/Detail/Core/ActorDestroyer.h>
#include <Theron/Detail/Messages/IMessage.h>
#include <Theron/Detail/Messages/MessageCreator.h>
#include <Theron/Detail/Threading/Lock.h>
#include <Theron/Detail/ThreadPool/WorkerContext.h>

#include <Theron/Defines.h>
#include <Theron/IAllocator.h>


namespace Theron
{
namespace Detail
{


/// Helper class that processes actor cores.
class ActorProcessor
{
public:

    typedef IntrusiveQueue<ActorCore> WorkQueue;

    /// Processes an actor core.
    /// \param workerContext Pointer to the context structure for the worker thread executing the call.
    /// \param workQueue Reference to a work queue on which the actor can be re-added for further processing.
    /// \param lock Reference to a lock on a mutex protecting access to the actor.
    /// \param actorCore Pointer to the core data of the actor that is to be processed.
    inline static void Process(
        WorkerContext *const workerContext,
        WorkQueue &workQueue,
        Lock &lock,
        ActorCore *const actorCore);
};


THERON_FORCEINLINE void ActorProcessor::Process(
    WorkerContext *const workerContext,
    WorkQueue &workQueue,
    Lock &lock,
    ActorCore *const actorCore)
{
    // Read an unprocessed message from the actor's message queue.
    // If there are no queued messages the returned pointer is null.
    IMessage *const message(actorCore->GetQueuedMessage());

    // We have to hold the lock while we check the referenced state, to make sure a
    // dereferencing ActorRef that decremented the reference count has finished accessing
    // it before we free it, in the case where the actor has become unreferenced.
    const bool referenced(actorCore->IsReferenced());

    // Set up a pointer to the context of this worker thread on the actor core.
    actorCore->SetWorkerContext(workerContext);

    // Increment the message processing counter for this worker thread, if we processed a message.
    // We do this while holding the lock so the counter can't be cleared just before we increment it. 
    // We exploit the fact that bools are 0 or 1 to avoid branches.
    workerContext->mMessageCount += static_cast<uint32_t>(message != 0);

    // An actor is still 'live' if it has unprocessed messages or is still referenced.
    // We intentionally use bitwise OR here to avoid branches.
    const bool stillAlive((message != 0) | referenced);

    lock.Unlock();

    // If the actor has a waiting message then process the message, even if the
    // actor is no longer referenced. This ensures messages sent to actors just
    // before they become unreferenced are still correctly processed.
    if (message)
    {
        // Update the actor's message handlers and handle the message.
        actorCore->ValidateHandlers();
        actorCore->ProcessMessage(message);

        // Destroy the message now that it's been read.
        IAllocator *const messageCache(&workerContext->mMessageCache);
        MessageCreator::Destroy(messageCache, message);
    }

    if (!stillAlive)
    {
        // Garbage collect the unreferenced actor.
        // This also frees any messages still in its queue.
        ActorDestroyer::DestroyActor(actorCore);
    }

    lock.Relock();

    if (stillAlive)
    {
        // Re-add the actor to the work queue if it still needs more processing,
        // including if it's unreferenced and we haven't destroyed it yet.
        // Note we intentionally use bitwise OR here to avoid branches.
        const bool stillDirty(actorCore->IsDirty());
        const bool stillReferenced(actorCore->IsReferenced());
        const bool stillHasMessages(actorCore->HasQueuedMessage());
        const bool stillNeedsProcessing(stillDirty | !stillReferenced | stillHasMessages);

        // Clear the worker context pointer, mark the actor as clean, and unschedule it.
        actorCore->SetWorkerContext(0);
        actorCore->CleanAndUnschedule();

        // Reschedule the actor if it still needs processing.
        if (stillNeedsProcessing)
        {
            actorCore->Schedule();
            workQueue.Push(actorCore);
        }
    }
}


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_THREADPOOL_ACTORPROCESSOR_H

