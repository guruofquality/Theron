// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_DETAIL_MAILBOXPROCESSOR_PROCESSOR_H
#define THERON_DETAIL_MAILBOXPROCESSOR_PROCESSOR_H


#include <Theron/Assert.h>
#include <Theron/BasicTypes.h>
#include <Theron/Counters.h>
#include <Theron/Defines.h>
#include <Theron/IAllocator.h>

#include <Theron/Detail/Allocators/CachingAllocator.h>
#include <Theron/Detail/Directory/Directory.h>
#include <Theron/Detail/Handlers/FallbackHandlerCollection.h>
#include <Theron/Detail/Mailboxes/Mailbox.h>
#include <Theron/Detail/MailboxProcessor/WorkQueue.h>
#include <Theron/Detail/Threading/Atomic.h>
#include <Theron/Detail/Threading/SpinLock.h>


#ifdef _MSC_VER
#pragma warning(push)
#pragma warning (disable:4324)  // structure was padded due to __declspec(align())
#endif //_MSC_VER


namespace Theron
{
namespace Detail
{


/**
Mailbox processor.
*/
class Processor
{
public:

    typedef void (* YieldFunction)(uint32_t &backoff);

    class YieldImplementation
    {
    public:

        inline explicit YieldImplementation() :
          mCounter(0),
          mYieldFunction(0)
        {
        }

        THERON_FORCEINLINE void SetYieldFunction(YieldFunction yieldFunction)
        {
            mYieldFunction = yieldFunction;
        }

        THERON_FORCEINLINE void Reset()
        {
            mCounter = 0;
        }

        THERON_FORCEINLINE void Execute()
        {
            THERON_ASSERT(mYieldFunction);
            (*mYieldFunction)(mCounter);
        }

    private:

        uint32_t mCounter;
        YieldFunction mYieldFunction;
    };

    /**
    Context structure holding data used in processing mailboxes.

    \note The members of a single context are all accessed only by one worker thread
    so we don't need to worry about shared writes, including false sharing.
    */
    struct Context
    {
        /**
        Constructor.
        */
        inline Context(
            Directory<Mailbox> *const mailboxes,
            SpinLock *const sharedWorkQueueSpinLock,
            FallbackHandlerCollection *const fallbackHandlers,
            IAllocator *const messageAllocator) :
          mMailboxes(mailboxes),
          mSharedWorkQueueSpinLock(sharedWorkQueueSpinLock),
          mWorkQueue(),
          mSharedWorkQueue(0),
          mLocalWorkQueue(0),
          mFallbackHandlers(fallbackHandlers),
          mMessageCache(messageAllocator)
        {
        }

        Directory<Mailbox> *const mMailboxes;                   ///< Pointer to the array of mailboxes serviced by this context.
        SpinLock *const mSharedWorkQueueSpinLock;               ///< Pointer to the spinlock protecting the shared work queue.
        WorkQueue mWorkQueue;                                   ///< Work queue owned by this context.
        WorkQueue *mSharedWorkQueue;                            ///< Pointer to the shared work queue.
        WorkQueue *mLocalWorkQueue;                             ///< Pointer to the local work queue.
        FallbackHandlerCollection *const mFallbackHandlers;     ///< Pointer to fallback handlers for undelivered messages.
        CachingAllocator<32> mMessageCache;                     ///< Per-thread cache of free message memory blocks.
        YieldImplementation mYield;                             ///< Thread yield strategy implementation.
        Atomic::UInt32 mCounters[MAX_COUNTERS];                 ///< Event counters.

    private:

        Context(const Context &other);
        Context &operator=(const Context &other);
    };

    /**
    Processes the work queue.
    */
    THERON_FORCEINLINE static void Process(Context *const context)
    {
        WorkQueue *const sharedWorkQueue(context->mSharedWorkQueue);
        Mailbox *mailbox(0);

        // Peek at the shared queue without locking it to check for work.
        if (!sharedWorkQueue->Empty())
        {
            SpinLock *const spinLock(context->mSharedWorkQueueSpinLock);

            // Lock the shared queue and try to pop the item we saw when we peeked.
            spinLock->Lock();
            mailbox = static_cast<Mailbox *>(sharedWorkQueue->Pop());
            spinLock->Unlock();

            if (mailbox)
            {
                // Non-inlined call.
                ProcessMailbox(context, mailbox);

                context->mYield.Reset();
                return;
            }
        }

        // Try to grab a mailbox from the local queue.
        mailbox = static_cast<Mailbox *>(context->mWorkQueue.Pop());
        if (mailbox)
        {
            // Non-inlined call.
            ProcessMailbox(context, mailbox);

            context->mYield.Reset();
            return;
        }

        // Yield to prevent busy-waiting on the work queue forever.
        context->mYield.Execute();
    }

    static void YieldPolite(uint32_t &counter);
    static void YieldStrong(uint32_t &counter);
    static void YieldAggressive(uint32_t &counter);

private:

    Processor();
    Processor(const Processor &other);
    Processor &operator=(const Processor &other);

    static void ProcessMailbox(Context *const context, Mailbox *const mailbox);
};


} // namespace Detail
} // namespace Theron


#ifdef _MSC_VER
#pragma warning(pop)
#endif //_MSC_VER


#endif // THERON_DETAIL_MAILBOXPROCESSOR_PROCESSOR_H
