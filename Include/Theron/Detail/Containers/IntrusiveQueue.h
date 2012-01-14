// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_DETAIL_CONTAINERS_INTRUSIVEQUEUE_H
#define THERON_DETAIL_CONTAINERS_INTRUSIVEQUEUE_H


#include <Theron/Detail/BasicTypes.h>
#include <Theron/Detail/Debug/Assert.h>

#include <Theron/Align.h>
#include <Theron/Defines.h>


namespace Theron
{
namespace Detail
{


/// Class template describing a generic queue.
/// \note The item type is the node type and is expected to expose SetNext and GetNext methods.
template <class ItemType>
class IntrusiveQueue
{
public:

    /// Constructor
    inline IntrusiveQueue();

    /// Destructor
    inline ~IntrusiveQueue();

    /// Returns true if the queue contains no items.
    inline bool Empty() const;

    /// Pushes an item onto the queue.
    inline void Push(ItemType *const item);

    /// Removes and returns the item at the front of the queue.
    inline ItemType *Pop();

private:

    IntrusiveQueue(const IntrusiveQueue &other);
    IntrusiveQueue &operator=(const IntrusiveQueue &other);

    ItemType *mFront;           ///< Pointer to the item at the front of the queue.
    ItemType *mBack;            ///< Pointer to the item at the back of the queue.
};


template <class ItemType>
THERON_FORCEINLINE IntrusiveQueue<ItemType>::IntrusiveQueue() :
  mFront(0),
  mBack(0)
{
}


template <class ItemType>
THERON_FORCEINLINE IntrusiveQueue<ItemType>::~IntrusiveQueue()
{
    // If the queue hasn't been emptied by the caller we'll leak the nodes.
    THERON_ASSERT(mFront == 0);
    THERON_ASSERT(mBack == 0);
}


template <class ItemType>
THERON_FORCEINLINE bool IntrusiveQueue<ItemType>::Empty() const
{
    return (mFront == 0);
}


template <class ItemType>
THERON_FORCEINLINE void IntrusiveQueue<ItemType>::Push(ItemType *const item)
{
    THERON_ASSERT(item);

    if (mBack)
    {
        THERON_ASSERT(mFront);
        mBack->SetNext(item);
    }
    else
    {
        THERON_ASSERT(mFront == 0);
        mFront = item;
    }

    item->SetNext(0);
    mBack = item;
}


template <class ItemType>
THERON_FORCEINLINE ItemType *IntrusiveQueue<ItemType>::Pop()
{
    ItemType *item(0);

    if (mFront)
    {
        item = mFront;
        ItemType *const next(mFront->GetNext());

        mFront = next;
        if (!next)
        {
            mBack = 0;
        }
    }

    return item;
}


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_CONTAINERS_INTRUSIVEQUEUE_H

