// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_DETAIL_SCHEDULER_WORKQUEUE_H
#define THERON_DETAIL_SCHEDULER_WORKQUEUE_H


#include <Theron/Align.h>
#include <Theron/Assert.h>
#include <Theron/BasicTypes.h>
#include <Theron/Defines.h>

#include <Theron/Detail/Scheduler/WorkItem.h>


#ifdef _MSC_VER
#pragma warning(push)
#pragma warning (disable:4324)  // structure was padded due to __declspec(align())
#endif //_MSC_VER


namespace Theron
{
namespace Detail
{


/**
A fast work queue.

The queue is unbounded and is based internally on a doubly-linked, circular, intrusive, linked list.
The head and tail are separated to separate the concerns of pushers and poppers.

\note The queue is intrusive and the item type is expected to derive from WorkItem.
*/
class WorkQueue
{
public:

    /**
    Constructor
    */
    inline WorkQueue();

    /**
    Destructor
    */
    inline ~WorkQueue();

    /**
    Returns true if the queue contains no items.
    Call this before calling Pop.
    */
    inline bool Empty() const;

    /**
    Pushes an item onto the queue.
    */
    inline void Push(WorkItem *const item);

    /**
    Removes and returns the item at the front of the queue.
    \note Returns a null pointer if the queue is empty.
    */
    inline WorkItem *Pop();

private:

    WorkQueue(const WorkQueue &other);
    WorkQueue &operator=(const WorkQueue &other);

    WorkItem mHead;                     ///< Dummy node that is always the head of the list.
    WorkItem mTail;                     ///< Dummy node that is always the tail of the list.
};


inline WorkQueue::WorkQueue() :
  mHead(),
  mTail()
{
    mHead.mNext = 0;
    mHead.mPrev = &mTail;

    mTail.mNext = &mHead;
    mTail.mPrev = 0;
}


inline WorkQueue::~WorkQueue()
{
    // If the queue hasn't been emptied by the caller we'll leak the nodes.
    THERON_ASSERT(mHead.mNext == 0);
    THERON_ASSERT(mHead.mPrev == &mTail);

    THERON_ASSERT(mTail.mNext == &mHead);
    THERON_ASSERT(mTail.mPrev == 0);
}


THERON_FORCEINLINE bool WorkQueue::Empty() const
{
    return (mHead.mPrev == &mTail);
}


THERON_FORCEINLINE void WorkQueue::Push(WorkItem *const item)
{
    // Doubly-linked list insert at back, ie. in front of the dummy tail.
    item->mPrev = &mTail;
    item->mNext = mTail.mNext;

    mTail.mNext->mPrev = item;
    mTail.mNext = item;
}


THERON_FORCEINLINE WorkItem *WorkQueue::Pop()
{
    WorkItem *const item(mHead.mPrev);

    // It's legal to call Pop when the queue is empty.
    if (item != &mTail)
    {
        // Doubly-linked list remove from front, ie. behind the dummy head.
        item->mPrev->mNext = &mHead;
        mHead.mPrev = item->mPrev;

        return item;
    }

    return 0;
}


} // namespace Detail
} // namespace Theron


#ifdef _MSC_VER
#pragma warning(pop)
#endif //_MSC_VER


#endif // THERON_DETAIL_SCHEDULER_WORKQUEUE_H
