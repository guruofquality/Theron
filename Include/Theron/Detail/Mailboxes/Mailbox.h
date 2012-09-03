// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_DETAIL_DIRECTORY_MAILBOXES_MAILBOX_H
#define THERON_DETAIL_DIRECTORY_MAILBOXES_MAILBOX_H


#include <Theron/BasicTypes.h>
#include <Theron/Address.h>
#include <Theron/Align.h>
#include <Theron/Defines.h>

#include <Theron/Detail/Containers/IntrusiveQueue.h>
#include <Theron/Detail/Messages/IMessage.h>
#include <Theron/Detail/Threading/SpinLock.h>


namespace Theron
{
namespace Detail
{


/**
An individual mailbox with a specific address.
*/
class THERON_PREALIGN(THERON_CACHELINE_ALIGNMENT) Mailbox : public IntrusiveQueue<Mailbox>::Node
{
public:

    /**
    Default constructor.
    */
    inline Mailbox();

    /**
    Gets the address of the mailbox.
    */
    inline Address GetAddress() const;

    /**
    Sets the address of the mailbox.
    */
    inline void SetAddress(const Address address);

    /**
    Lock the mailbox, acquiring exclusive access.
    */
    inline void Lock() const;

    /**
    Unlock the mailbox, relinquishing exclusive access.
    */
    inline void Unlock() const;

    /**
    Returns true if the mailbox contains no messages.
    */
    inline bool Empty() const;

    /**
    Pushes a message into the mailbox.
    */
    inline void Push(IMessage *const message);

    /**
    Peeks at the first message in the mailbox.
    The message is inspected without actually being removed from the mailbox.
    \return Pointer to a message, if the mailbox is non-empty, otherwise zero.
    */
    inline IMessage *Front() const;

    /**
    Pops the first message from the mailbox.
    \return Pointer to a message, if the mailbox is non-empty, otherwise zero.
    */
    inline IMessage *Pop();

    /**
    Returns the number of messages currently queued in the mailbox.
    */
    inline uint32_t Count() const;

private:

    typedef IntrusiveQueue<IMessage> MessageQueue;

    Address mAddress;                           ///< Unique address of this mailbox.
    mutable SpinLock mSpinLock;                 ///< Thread synchronization object protecting the mailbox.
    MessageQueue mQueue;                        ///< Queue of messages in this mailbox.
    uint32_t mMessageCount;                     ///< Size of the message queue.

} THERON_POSTALIGN(THERON_CACHELINE_ALIGNMENT);


inline Mailbox::Mailbox() :
  mAddress(),
  mSpinLock(),
  mQueue(),
  mMessageCount(0)
{
}


THERON_FORCEINLINE Address Mailbox::GetAddress() const
{
    return mAddress;
}


inline void Mailbox::SetAddress(const Address address)
{
    mAddress = address;
}


THERON_FORCEINLINE void Mailbox::Lock() const
{
    mSpinLock.Lock();
}


THERON_FORCEINLINE void Mailbox::Unlock() const
{
    mSpinLock.Unlock();
}


THERON_FORCEINLINE bool Mailbox::Empty() const
{
    return mQueue.Empty();
}


THERON_FORCEINLINE void Mailbox::Push(IMessage *const message)
{
    mQueue.Push(message);
    ++mMessageCount;
}


THERON_FORCEINLINE IMessage *Mailbox::Front() const
{
    return mQueue.Front();
}


THERON_FORCEINLINE IMessage *Mailbox::Pop()
{
    --mMessageCount;
    return mQueue.Pop();
}


THERON_FORCEINLINE uint32_t Mailbox::Count() const
{
    return mMessageCount;
}


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_DIRECTORY_MAILBOXES_MAILBOX_H

