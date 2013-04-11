// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_DETAIL_SCHEDULER_MAILBOXCONTEXT_H
#define THERON_DETAIL_SCHEDULER_MAILBOXCONTEXT_H


#include <Theron/BasicTypes.h>
#include <Theron/Counters.h>
#include <Theron/Defines.h>
#include <Theron/IAllocator.h>

#include <Theron/Detail/Allocators/CachingAllocator.h>
#include <Theron/Detail/Directory/Directory.h>
#include <Theron/Detail/Handlers/FallbackHandlerCollection.h>
#include <Theron/Detail/Mailboxes/Mailbox.h>
#include <Theron/Detail/Scheduler/IScheduler.h>
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
Context structure holding data used by a worker thread to process mailboxes.

\note The members of a single context are all accessed only by one worker thread
so we don't need to worry about shared writes, including false sharing.
*/
struct MailboxContext
{
    /**
    Constructor.
    */
    inline MailboxContext(IAllocator *const messageAllocator) :
      mMailboxes(0),
      mFallbackHandlers(0),
      mMessageCache(messageAllocator),
      mScheduler(0)
    {
    }

    Directory<Mailbox> *mMailboxes;                         ///< Pointer to the array of mailboxes serviced by this context.
    FallbackHandlerCollection *mFallbackHandlers;           ///< Pointer to fallback handlers for undelivered messages.
    CachingAllocator<32> mMessageCache;                     ///< Per-thread cache of free message memory blocks.
    IScheduler *mScheduler;                                 ///< Pointer to a per-thread mailbox scheduler.
    Atomic::UInt32 mCounters[MAX_COUNTERS];                 ///< Event counters.

private:

    MailboxContext(const MailboxContext &other);
    MailboxContext &operator=(const MailboxContext &other);
};


} // namespace Detail
} // namespace Theron


#ifdef _MSC_VER
#pragma warning(pop)
#endif //_MSC_VER


#endif // THERON_DETAIL_SCHEDULER_MAILBOXCONTEXT_H
