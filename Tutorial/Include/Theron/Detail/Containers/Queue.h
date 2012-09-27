// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_DETAIL_CONTAINERS_QUEUE_H
#define THERON_DETAIL_CONTAINERS_QUEUE_H


#include <Theron/Assert.h>
#include <Theron/BasicTypes.h>
#include <Theron/Defines.h>


namespace Theron
{
namespace Detail
{


/**
Class template describing a generic queue.
\note The item type is the node type and is expected to derive from Node.
*/
template <class ItemType>
class Queue
{
public:

    /**
    Baseclass that adds link members to node types that derive from it.
    */
    class Node
    {
    public:

        Node *mFrwd;                ///< Pointer to the next item in the circular list.
        Node *mBack;                ///< Pointer to the previous item in the circular list.
    };

    /**
    Constructor
    */
    inline Queue();

    /**
    Destructor
    */
    inline ~Queue();

    /**
    Returns true if the queue contains no items.
    Call this before calling Pop.
    */
    inline bool Empty() const;

    /**
    Pushes an item onto the queue.
    */
    inline void Push(ItemType *const item);

    /**
    Peeks at the item at the front of the queue without removing it.
    \note Returns a null pointer if the queue is empty.
    */
    inline ItemType *Front() const;

    /**
    Removes and returns the item at the front of the queue.
    \note Returns a null pointer if the queue is empty.
    */
    inline ItemType *Pop();

private:

    Queue(const Queue &other);
    Queue &operator=(const Queue &other);

    Node mHead;                 ///< Dummy node that is always the head and tail of the circular list.
};


template <class ItemType>
THERON_FORCEINLINE Queue<ItemType>::Queue() : mHead()
{
    mHead.mFrwd = &mHead;
    mHead.mBack = &mHead;
}


template <class ItemType>
THERON_FORCEINLINE Queue<ItemType>::~Queue()
{
    // If the queue hasn't been emptied by the caller we'll leak the nodes.
    THERON_ASSERT(mHead.mFrwd == &mHead);
    THERON_ASSERT(mHead.mBack == &mHead);
}


template <class ItemType>
THERON_FORCEINLINE bool Queue<ItemType>::Empty() const
{
    return (mHead.mFrwd == &mHead);
}


template <class ItemType>
THERON_FORCEINLINE void Queue<ItemType>::Push(ItemType *const item)
{
    THERON_ASSERT(item);

#if THERON_DEBUG

    // Check that the pushed item isn't already in the queue.
    for (Node *node(mHead.mFrwd); node != &mHead; node = node->mFrwd)
    {
        THERON_ASSERT(node != item);
    }

#endif

    // Circular doubly-linked list insert at back.
    item->mFrwd = &mHead;
    item->mBack = mHead.mBack;

    mHead.mBack->mFrwd = item;
    mHead.mBack = item;
}


template <class ItemType>
THERON_FORCEINLINE ItemType *Queue<ItemType>::Front() const
{
    // It's legal to call Front when the queue is empty.
    if (mHead.mFrwd != &mHead)
    {
        return static_cast<ItemType *>(mHead.mFrwd);
    }

    return 0;
}


template <class ItemType>
THERON_FORCEINLINE ItemType *Queue<ItemType>::Pop()
{
    // It's legal to call Pop when the queue is empty.
    if (mHead.mFrwd != &mHead)
    {
        Node *const item(mHead.mFrwd);

        // Circular doubly-linked list remove from front.
        item->mFrwd->mBack = &mHead;
        mHead.mFrwd = item->mFrwd;

        return static_cast<ItemType *>(item);
    }

    return 0;
}


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_CONTAINERS_QUEUE_H
