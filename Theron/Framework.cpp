// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#include <new>

#include <Theron/Actor.h>
#include <Theron/Assert.h>
#include <Theron/AllocatorManager.h>
#include <Theron/Defines.h>
#include <Theron/EndPoint.h>
#include <Theron/Framework.h>
#include <Theron/IAllocator.h>

#include <Theron/Detail/Directory/StaticDirectory.h>
#include <Theron/Detail/Scheduler/BlockingQueue.h>
#include <Theron/Detail/Scheduler/NonBlockingQueue.h>
#include <Theron/Detail/Scheduler/Scheduler.h>
#include <Theron/Detail/Network/Index.h>
#include <Theron/Detail/Network/NameGenerator.h>
#include <Theron/Detail/Strings/String.h>
#include <Theron/Detail/Threading/Utils.h>


#ifdef _MSC_VER
#pragma warning(push)
#pragma warning (disable:4996)  // function or variable may be unsafe
#endif //_MSC_VER


namespace Theron
{


void Framework::Initialize()
{
    mScheduler = CreateScheduler();

    // Set up the scheduler.
    mScheduler->Initialize(mParams.mThreadCount);

    // Set up the default fallback handler, which catches and reports undelivered messages.
    SetFallbackHandler(&mDefaultFallbackHandler, &Detail::DefaultFallbackHandler::Handle);
     
    // Register the framework and get a non-zero index for it, unique within the local process.
    mIndex = Detail::StaticDirectory<Framework>::Register(this);
    THERON_ASSERT(mIndex);

    // If the framework name wasn't set explicitly then generate a default name.
    if (mName.IsNull())
    {
        char buffer[16];
        Detail::NameGenerator::Generate(buffer, mIndex);
        mName = Detail::String(buffer);
    }
}


void Framework::Release()
{
    // Deregister the framework.
    Detail::StaticDirectory<Framework>::Deregister(mIndex);

    mScheduler->Release();
    DestroyScheduler(mScheduler);
    mScheduler = 0;
}


Detail::IScheduler *Framework::CreateScheduler()
{
    typedef Detail::Scheduler<Detail::BlockingQueue> BlockingScheduler;
    typedef Detail::Scheduler<Detail::NonBlockingQueue> NonBlockingScheduler;

    IAllocator *const allocator(AllocatorManager::Instance().GetAllocator());
    void *schedulerMemory(0);

    if (mParams.mYieldStrategy == YIELD_STRATEGY_BLOCKING)
    {
        schedulerMemory = allocator->AllocateAligned(
            sizeof(BlockingScheduler),
            THERON_CACHELINE_ALIGNMENT);
    }
    else
    {
        schedulerMemory = allocator->AllocateAligned(
            sizeof(NonBlockingScheduler),
            THERON_CACHELINE_ALIGNMENT);
    }

    THERON_ASSERT_MSG(schedulerMemory, "Failed to allocate scheduler");

    if (mParams.mYieldStrategy == YIELD_STRATEGY_BLOCKING)
    {
        return new (schedulerMemory) BlockingScheduler(
            &mMailboxes,
            &mFallbackHandlers,
            &mMessageAllocator,
            mParams.mNodeMask,
            mParams.mProcessorMask,        
            mParams.mYieldStrategy);
    }
    else
    {
        return new (schedulerMemory) NonBlockingScheduler(
            &mMailboxes,
            &mFallbackHandlers,
            &mMessageAllocator,
            mParams.mNodeMask,
            mParams.mProcessorMask,        
            mParams.mYieldStrategy);
    }
}


void Framework::DestroyScheduler(Detail::IScheduler *const scheduler)
{
    IAllocator *const allocator(AllocatorManager::Instance().GetAllocator());

    scheduler->~IScheduler();
    allocator->Free(scheduler);
}


void Framework::RegisterActor(Actor *const actor, const char *const name)
{
    // Allocate an unused mailbox.
    const uint32_t mailboxIndex(mMailboxes.Allocate());
    Detail::Mailbox &mailbox(mMailboxes.GetEntry(mailboxIndex));

    // Use the provided name for the actor if one was provided.
    Detail::String mailboxName(name);
    if (name == 0)
    {
        char rawName[16];
        Detail::NameGenerator::Generate(rawName, mailboxIndex);

        const char *endPointName(0);
        if (mEndPoint)
        {
            endPointName = mEndPoint->GetName();        
        }

        char scopedName[256];
        Detail::NameGenerator::Combine(
            scopedName,
            256,
            rawName,
            mName.GetValue(),
            endPointName);

        mailboxName = Detail::String(scopedName);
    }

    // Name the mailbox and register the actor.
    mailbox.Lock();
    mailbox.SetName(mailboxName);
    mailbox.RegisterActor(actor);
    mailbox.Unlock();

    // Create the unique address of the mailbox.
    // Its a pair comprising the framework index and the mailbox index within the framework.
    const Detail::Index index(mIndex, mailboxIndex);
    const Address mailboxAddress(mailboxName, index);

    // Set the actor's mailbox address.
    // The address contains the index of the framework and the index of the mailbox within the framework.
    actor->mAddress = mailboxAddress;

    if (mEndPoint)
    {
        // Check that no mailbox with this name already exists.
        Detail::Index dummy;
        if (mEndPoint->Lookup(mailboxName, dummy))
        {
            THERON_FAIL_MSG("Can't create two actors or receivers with the same name");
        }
        
        // Register the mailbox with the endPoint so it can be found using its name.
        if (!mEndPoint->Register(mailboxName, index))
        {
            THERON_FAIL_MSG("Failed to register actor with the network endpoint");
        }
    }
}


void Framework::DeregisterActor(Actor *const actor)
{
    const Address address(actor->GetAddress());
    const Detail::String &mailboxName(address.GetName());

    // Deregister the mailbox with the endPoint so it can't be found anymore.
    if (mEndPoint)
    {
        mEndPoint->Deregister(mailboxName);
    }

    // Deregister the actor, so that the worker threads will leave it alone.
    const uint32_t mailboxIndex(address.AsInteger());
    Detail::Mailbox &mailbox(mMailboxes.GetEntry(mailboxIndex));

    // If the entry is pinned then we have to wait for it to be unpinned.
    bool deregistered(false);
    uint32_t backoff(0);

    while (!deregistered)
    {
        mailbox.Lock();

        if (!mailbox.IsPinned())
        {
            mailbox.DeregisterActor();
            deregistered = true;
        }

        mailbox.Unlock();

        Detail::Utils::Backoff(backoff);
    }
}


} // namespace Theron


#ifdef _MSC_VER
#pragma warning(pop)
#endif //_MSC_VER
