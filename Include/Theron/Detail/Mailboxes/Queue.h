// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_DETAIL_MAILBOXES_QUEUE_H
#define THERON_DETAIL_MAILBOXES_QUEUE_H


#include <Theron/Align.h>
#include <Theron/Assert.h>
#include <Theron/BasicTypes.h>
#include <Theron/Defines.h>

#include <Theron/Detail/Containers/IntrusiveQueue.h>
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
Class template describing a generic unbounded queue.
*/
template <class ITEMTYPE>
class THERON_PREALIGN(THERON_CACHELINE_ALIGNMENT) Queue
{
public:

    typedef ITEMTYPE ItemType;

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
    */
    inline bool Empty() const;

    /**
    Pushes an entry onto the back of the queue.
    */
    inline void Push(ItemType *const item);

    /**
    Pops an entry off the front of the queue, if the queue is non-empty.
    \return A pointer to the popped item, or zero if the queue was empty.
    */
    inline ItemType *Pop();

private:

    Queue(const Queue &other);
    Queue &operator=(const Queue &other);

    mutable SpinLock mSpinLock;         ///< Protects access to the internal queue.
    IntrusiveQueue<ItemType> mQueue;    ///< Internal non-threadsafe queue implementation.

} THERON_POSTALIGN(THERON_CACHELINE_ALIGNMENT);


template <class ItemType>
inline Queue<ItemType>::Queue() :
  mSpinLock(),
  mQueue()
{
}


template <class ItemType>
inline Queue<ItemType>::~Queue()
{
    // If the queue hasn't been emptied by the caller we'll leak the nodes.
    // This can happen if the framework is allowed to destruct while messages are still in flight.
    THERON_ASSERT(mQueue.Empty());
}


template <class ItemType>
THERON_FORCEINLINE bool Queue<ItemType>::Empty() const
{
    bool empty(false);

    mSpinLock.Lock();
    empty = mQueue.Empty();
    mSpinLock.Unlock();

    return empty;
}


template <class ItemType>
THERON_FORCEINLINE void Queue<ItemType>::Push(ItemType *const item)
{
    mSpinLock.Lock();
    mQueue.Push(item);
    mSpinLock.Unlock();
}


template <class ItemType>
THERON_FORCEINLINE ItemType *Queue<ItemType>::Pop()
{
    ItemType *item(0);

    mSpinLock.Lock();
    item = mQueue.Pop();
    mSpinLock.Unlock();

    return item;
}


} // namespace Detail
} // namespace Theron


#ifdef _MSC_VER
#pragma warning(pop)
#endif //_MSC_VER


#endif // THERON_DETAIL_MAILBOXES_QUEUE_H
