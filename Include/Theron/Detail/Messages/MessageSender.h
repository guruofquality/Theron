// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_DETAIL_MESSAGES_MESSAGESENDER_H
#define THERON_DETAIL_MESSAGES_MESSAGESENDER_H


#include <Theron/Detail/Directory/Directory.h>
#include <Theron/Detail/Messages/IMessage.h>
#include <Theron/Detail/Messages/MessageCreator.h>
#include <Theron/Detail/Threading/Lock.h>

#include <Theron/Address.h>
#include <Theron/Defines.h>


namespace Theron
{


class Framework;


namespace Detail
{


/// Helper class that knows how to send messages.
/// The methods of this class represent non-inlined call points that break cyclic header
/// dependencies and reduce code bloat from excessive inlining.
class MessageSender
{
public:

    /// Sends the given value as a message from an address in the given framework
    /// to some other address. 
    template <class ValueType>
    inline static bool Send(
        const Framework *const framework,
        const ValueType &value,
        const Address &from,
        const Address &to);

    /// Sends the given value as a message from an address in the given framework
    /// to some other address, without waking a worker thread to process it.
    template <class ValueType>
    inline static bool TailSend(
        const Framework *const framework,
        const ValueType &value,
        const Address &from,
        const Address &to);

private:

    /// Delivers the given message to the given address.
    /// This is a non-inlined called function to avoid code bloat.
    static bool Deliver(const Framework *const framework, IMessage *const message, const Address &address);

    /// Delivers the given message to the given address, without waking a worker thread to process it.
    /// This is a non-inlined called function to avoid code bloat.
    static bool TailDeliver(const Framework *const framework, IMessage *const message, const Address &address);
};


template <class ValueType>
THERON_FORCEINLINE bool MessageSender::Send(
    const Framework *const framework,
    const ValueType &value,
    const Address &from,
    const Address &to)
{
    IMessage *message(0);

    {
        // Allocate a message. It'll be deleted by the target after it's been handled.
        // The directory lock is used to protect the global free list.
        Lock lock(Directory::GetMutex());
        message = MessageCreator::Create(value, from);
    }

    if (message != 0)
    {
        // This call is non-inlined to reduce code bloat.
        if (Deliver(framework, message, to))
        {
            return true;
        }

        // If the message wasn't delivered we need to delete it ourselves.
        Lock lock(Directory::GetMutex());
        MessageCreator::Destroy(message);
    }

    return false;
}


template <class ValueType>
THERON_FORCEINLINE bool MessageSender::TailSend(
    const Framework *const framework,
    const ValueType &value,
    const Address &from,
    const Address &to)
{
    IMessage *message(0);

    {
        // Allocate a message. It'll be deleted by the target after it's been handled.
        // The directory lock is used to protect the global free list.
        Lock lock(Directory::GetMutex());
        message = MessageCreator::Create(value, from);
    }

    if (message != 0)
    {
        // This call is non-inlined to reduce code bloat.
        // This 'tail' call doesn't wake a worker thread to process the message.
        if (TailDeliver(framework, message, to))
        {
            return true;
        }

        // If the message wasn't delivered we need to delete it ourselves.
        Lock lock(Directory::GetMutex());
        MessageCreator::Destroy(message);
    }

    return false;
}


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_MESSAGES_MESSAGESENDER_H

