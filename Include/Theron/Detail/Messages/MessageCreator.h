// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_DETAIL_MESSAGES_MESSAGECREATOR_H
#define THERON_DETAIL_MESSAGES_MESSAGECREATOR_H


#include <Theron/Detail/BasicTypes.h>
#include <Theron/Detail/MessageCache/MessageCache.h>
#include <Theron/Detail/Messages/IMessage.h>
#include <Theron/Detail/Messages/Message.h>

#include <Theron/Address.h>
#include <Theron/Defines.h>


namespace Theron
{
namespace Detail
{


/// Helper class that constructs and destroys Theron's internal message objects.
class MessageCreator
{
public:

    /// Allocates and constructs a message with the given value and from address.
    template <class ValueType>
    inline static Message<ValueType> *Create(const ValueType &value, const Address &from);

    /// Destructs and frees a message of unknown type referenced by an interface pointer.
    inline static void Destroy(IMessage *const message);
};



template <class ValueType>
THERON_FORCEINLINE Message<ValueType> *MessageCreator::Create(const ValueType &value, const Address &from)
{
    typedef Message<ValueType> MessageType;

    const uint32_t blockSize(MessageType::GetSize());
    const uint32_t blockAlignment(MessageType::GetAlignment());

    // Allocate a message. It'll be deleted by the actor after it's been handled.
    // We allocate a block from the global free list for caching of common allocations.
    // The free list is thread-safe so we don't need to lock it ourselves.
    void *const block = MessageCache::Instance().Allocate(blockSize, blockAlignment);
    if (block)
    {
        return MessageType::Initialize(block, value, from);
    }

    return 0;
}


THERON_FORCEINLINE void MessageCreator::Destroy(IMessage *const message)
{
    // Call release on the message to give it chance to destruct its value type.
    message->Release();

    // Return the block to the global free list.
    MessageCache::Instance().Free(message->GetBlock(), message->GetBlockSize());
}


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_MESSAGES_MESSAGECREATOR_H

