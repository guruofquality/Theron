// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#include <Theron/Receiver.h>

#include <Theron/Detail/Directory/StaticDirectory.h>


namespace Theron
{


Receiver::Receiver() :
  mAddress(),
  mMessageHandlers(),
  mSpinLock(),
  mMessagesReceived(0)
{
    // Register this receiver, claiming a unique address for this receiver.
    const uint32_t index(Detail::StaticDirectory<Receiver>::Register(this));

    // Receivers are identified by a framework index of zero.
    mAddress = Address(0, index);

    // Register the receiver at its claimed address.
    Detail::Entry &entry(Detail::StaticDirectory<Receiver>::GetEntry(mAddress.AsInteger()));

    entry.Lock();
    entry.SetEntity(this);
    entry.Unlock();
}


Receiver::~Receiver()
{
    // Deregister the receiver, so that the worker threads will leave it alone.
    Detail::StaticDirectory<Receiver>::Deregister(GetAddress().AsInteger());

    mSpinLock.Lock();

    // Free all currently allocated handler objects.
    while (Detail::IReceiverHandler *const handler = mMessageHandlers.Front())
    {
        mMessageHandlers.Remove(handler);
        AllocatorManager::Instance().GetAllocator()->Free(handler);
    }

    mSpinLock.Unlock();
}


} // namespace Theron


