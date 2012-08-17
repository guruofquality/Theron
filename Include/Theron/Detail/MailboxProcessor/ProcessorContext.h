// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_DETAIL_MAILBOXPROCESSOR_PROCESSORCONTEXT_H
#define THERON_DETAIL_MAILBOXPROCESSOR_PROCESSORCONTEXT_H


#include <Theron/BasicTypes.h>
#include <Theron/Defines.h>
#include <Theron/IAllocator.h>

#include <Theron/Detail/Directory/Directory.h>
#include <Theron/Detail/Directory/Entry.h>
#include <Theron/Detail/Handlers/FallbackHandlerCollection.h>
#include <Theron/Detail/Mailboxes/Mailbox.h>
#include <Theron/Detail/Mailboxes/Queue.h>


namespace Theron
{
namespace Detail
{


/**
Context structure holding data used in processing mailboxes.
*/
struct ProcessorContext
{
    /**
    Constructor.
    */
    inline ProcessorContext(
        Directory<Entry> *const actorDirectory,
        Directory<Mailbox> *const mailboxes,
        Queue<Mailbox> *const workQueue,
        FallbackHandlerCollection *const fallbackHandlers,
        IAllocator *const messageAllocator) :
      mActorDirectory(actorDirectory),
      mMailboxes(mailboxes),
      mWorkQueue(workQueue),
      mFallbackHandlers(fallbackHandlers),
      mMessageAllocator(messageAllocator),
      mMessageCount(0),
      mPulseCount(0),
      mWakeCount(0)
    {
    }

    Directory<Entry> *mActorDirectory;                      ///< Pointer to the directory of actors owned by this context.
    Directory<Mailbox> *mMailboxes;                         ///< Pointer to the array of mailboxes serviced by this context.
    Queue<Mailbox> *mWorkQueue;                             ///< Pointer to the work queue serviced by this context.
    FallbackHandlerCollection *mFallbackHandlers;           ///< Pointer to fallback handlers for undelivered messages.
    IAllocator *mMessageAllocator;                          ///< Pointer to a message memory block allocator.
    uint32_t mMessageCount;                                 ///< Number of messages processed within this context.
    uint32_t mPulseCount;                                   ///< Number of times the threadpool was pulsed within this context.
    uint32_t mWakeCount;                                    ///< Number of times a thread was woken within this context.

private:

    ProcessorContext(const ProcessorContext &other);
    ProcessorContext &operator=(const ProcessorContext &other);
};


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_MAILBOXPROCESSOR_PROCESSORCONTEXT_H
