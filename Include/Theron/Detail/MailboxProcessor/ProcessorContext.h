// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_DETAIL_MAILBOXPROCESSOR_PROCESSORCONTEXT_H
#define THERON_DETAIL_MAILBOXPROCESSOR_PROCESSORCONTEXT_H


#include <Theron/BasicTypes.h>
#include <Theron/Counters.h>
#include <Theron/Defines.h>
#include <Theron/IAllocator.h>

#include <Theron/Detail/Directory/Directory.h>
#include <Theron/Detail/Directory/Entry.h>
#include <Theron/Detail/Handlers/FallbackHandlerCollection.h>
#include <Theron/Detail/Mailboxes/Mailbox.h>
#include <Theron/Detail/Mailboxes/Queue.h>
#include <Theron/Detail/Threading/Atomic.h>


#ifdef _MSC_VER
#pragma warning(push)
#pragma warning (disable:4324)  // structure was padded due to __declspec(align())
#endif //_MSC_VER


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
      mMessageAllocator(messageAllocator)
    {
    }

    Directory<Entry> *mActorDirectory;                      ///< Pointer to the directory of actors owned by this context.
    Directory<Mailbox> *mMailboxes;                         ///< Pointer to the array of mailboxes serviced by this context.
    Queue<Mailbox> *mWorkQueue;                             ///< Pointer to the work queue serviced by this context.
    FallbackHandlerCollection *mFallbackHandlers;           ///< Pointer to fallback handlers for undelivered messages.
    IAllocator *mMessageAllocator;                          ///< Pointer to a message memory block allocator.
    Atomic::UInt32 mCounters[MAX_COUNTERS];                 ///< Events counters.

private:

    ProcessorContext(const ProcessorContext &other);
    ProcessorContext &operator=(const ProcessorContext &other);
};


} // namespace Detail
} // namespace Theron


#ifdef _MSC_VER
#pragma warning(pop)
#endif //_MSC_VER


#endif // THERON_DETAIL_MAILBOXPROCESSOR_PROCESSORCONTEXT_H
