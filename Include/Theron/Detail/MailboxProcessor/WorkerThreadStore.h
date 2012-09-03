// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_DETAIL_MAILBOXPROCESSOR_WORKERTHREADSTORE_H
#define THERON_DETAIL_MAILBOXPROCESSOR_WORKERTHREADSTORE_H


#include <Theron/BasicTypes.h>
#include <Theron/Defines.h>
#include <Theron/IAllocator.h>

#include <Theron/Detail/Allocators/CachingAllocator.h>
#include <Theron/Detail/Directory/Directory.h>
#include <Theron/Detail/Directory/Entry.h>
#include <Theron/Detail/Mailboxes/Mailbox.h>
#include <Theron/Detail/Mailboxes/Queue.h>
#include <Theron/Detail/MailboxProcessor/ProcessorContext.h>


namespace Theron
{
namespace Detail
{


/**
Per-thread data store associated with worker threads.
*/
struct WorkerThreadStore
{
    typedef CachingAllocator<32> MessageCache;

    /**
    \brief Constructor.

    The contained processor context is initialized using the provided per-framework context.

    The per-thread message cache is initialized as a wrapper around the per-framework message cache
    referenced by the provided per-framework processor context.

    The contained processor context is initialized with the local message cache rather than the
    message allocator of the owning framework.
    */
    inline WorkerThreadStore(
        Directory<Entry> *const actorDirectory,
        Directory<Mailbox> *const mailboxes,
        Queue<Mailbox> *const workQueue,
        FallbackHandlerCollection *const fallbackHandlers,
        IAllocator *const messageAllocator) :
      mMessageCache(messageAllocator),
      mProcessorContext(
        actorDirectory,
        mailboxes,
        workQueue,
        fallbackHandlers,
        &mMessageCache)
    {
    }

    MessageCache mMessageCache;                     ///< Per-thread cache of free message memory blocks.
    ProcessorContext mProcessorContext;             ///< Processor context owned and used by this thread.
};


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_MAILBOXPROCESSOR_WORKERTHREADSTORE_H
