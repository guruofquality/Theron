// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_DETAIL_MAILBOXPROCESSOR_WORKITEM_H
#define THERON_DETAIL_MAILBOXPROCESSOR_WORKITEM_H


#include <Theron/Align.h>
#include <Theron/Defines.h>


#ifdef _MSC_VER
#pragma warning(push)
#pragma warning (disable:4324)  // structure was padded due to __declspec(align())
#endif //_MSC_VER


namespace Theron
{
namespace Detail
{


/**
Baseclass that adds link members to node types that derive from it.
*/
class THERON_PREALIGN(THERON_CACHELINE_ALIGNMENT) WorkItem
{
public:

    inline WorkItem() : mNext(0), mPrev(0)
    {
    }

    WorkItem *mNext;                ///< Pointer to the next item in the list.
    WorkItem *mPrev;                ///< Pointer to the previous item in the list.

private:

    WorkItem(const WorkItem &other);
    WorkItem &operator=(const WorkItem &other);

} THERON_POSTALIGN(THERON_CACHELINE_ALIGNMENT);


} // namespace Detail
} // namespace Theron


#ifdef _MSC_VER
#pragma warning(pop)
#endif //_MSC_VER


#endif // THERON_DETAIL_MAILBOXPROCESSOR_WORKITEM_H
