// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#include <Theron/Detail/Directory/ReceiverDirectory.h>
#include <Theron/Detail/Debug/Assert.h>

#include <Theron/Receiver.h>


namespace Theron
{
namespace Detail
{


ReceiverDirectory ReceiverDirectory::smInstance;


Address ReceiverDirectory::RegisterReceiver(Receiver *const receiver)
{
    uint32_t index(0);
    if (mReceiverPool.Allocate(index))
    {
        // Fill in the allocated entry with the pointer to the receiver.
        void *const entry(mReceiverPool.GetEntry(index));
        if (entry)
        {
            Receiver **const p = reinterpret_cast<Receiver **>(entry);
            *p = receiver;

            return Address::MakeReceiverAddress(index);
        }
    }

    return Address::Null();
}


bool ReceiverDirectory::DeregisterReceiver(const Address &address)
{
    // Receiver addresses are flagged with a bit flag in the index fields.
    THERON_ASSERT(!Address::IsActorAddress(address));
    const uint32_t index(address.GetIndex());

    if (mReceiverPool.Free(index))
    {
        return true;
    }

    return false;
}


Receiver *ReceiverDirectory::GetReceiver(const Address &address) const
{
    // Receiver addresses are flagged with a bit flag in the index fields.
    THERON_ASSERT(!Address::IsActorAddress(address));
    const uint32_t index(address.GetIndex());

    // The entry in the receiver pool is memory for a receiver pointer.
    void *const entry(mReceiverPool.GetEntry(index));
    if (entry)
    {
        Receiver **const p = reinterpret_cast<Receiver **>(entry);
        Receiver *const receiver(*p);

        // Reject the receiver if its address (ie. sequence number) doesn't match.
        // This guards against new receivers constructed at the same indices as old ones.
        if (receiver->GetAddress() == address)
        {
            return receiver;
        }
    }

    return 0;
}


} // namespace Detail
} // namespace Theron


