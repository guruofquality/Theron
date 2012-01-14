// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_DETAIL_CONTAINERS_INTRUSIVELIST_H
#define THERON_DETAIL_CONTAINERS_INTRUSIVELIST_H


#include <Theron/Detail/BasicTypes.h>
#include <Theron/Detail/Debug/Assert.h>

#include <Theron/Defines.h>


namespace Theron
{
namespace Detail
{


/// Class template implementing a generic unsorted list.
/// \note The item type is the node type and is expected to expose SetNext and GetNext methods.
template <class ItemType>
class IntrusiveList
{
public:

    /// Iterator type.
    class Iterator
    {
    public:

        /// Default constructor
        /// Constructs an invalid iterator which must be assigned before being used.
        THERON_FORCEINLINE Iterator() : mNode(0)
        {
        }

        /// Constructor
        /// \param node The list node that the iterator initially references.
        THERON_FORCEINLINE explicit Iterator(ItemType *const node) : mNode(node)
        {
        }

        /// Copy constructor.
        THERON_FORCEINLINE Iterator(const Iterator &other) : mNode(other.mNode)
        {
        }

        /// Assignment operator.
        THERON_FORCEINLINE Iterator &operator=(const Iterator &other)
        {
            mNode = other.mNode;
            return *this;
        }

        /// Dereference operator. Returns a pointer to the list item that the iterator currently references.
        /// \return A pointer to the list item referenced by the iterator.
        /// \note If the iterator doesn't reference a valid item then the result is zero.
        THERON_FORCEINLINE ItemType *operator*() const
        {
            return mNode;
        }

        /// Pre-increment operator.
        /// Moves the iterator to reference the next node in the list.
        /// If the iterator currently references the last item in the list then the iterator
        /// beomes equal to End after incrementation.
        /// \note If the iterator is already equal to End then the result is undefined.
        THERON_FORCEINLINE void operator++()
        {
            THERON_ASSERT(mNode);
            mNode = mNode->GetNext();
        }

        /// Equality operator. Compares two iterators for equality.
        /// \param other Another iterator whose value is to be compared to this one.
        /// \return True if the iterators reference the same item in the same list, otherwise false.
        THERON_FORCEINLINE bool operator==(const Iterator &other) const
        {
            return (mNode == other.mNode);
        }

        /// Inequality operator. Compares two iterators for inequality.
        /// \param other Another iterator whose value is to be compared to this one.
        /// \return True if the iterators reference different items or different lists, otherwise false.
        THERON_FORCEINLINE bool operator!=(const Iterator &other) const
        {
            return (mNode != other.mNode);
        }

    protected:

        ItemType *mNode;        ///< Pointer to the list node that the iterator references.
    };

    /// Constructor
    inline IntrusiveList();
    
    /// Destructor
    inline ~IntrusiveList();

    /// Returns an iterator referencing the first item in the list.
    /// \return An iterator referencing the first item in the list, or End if the list is empty.
    inline Iterator Begin() const;

    /// Returns an iterator referencing the end of the list.
    /// Incrementing an iterator that references the last item in the list results in the
    /// iterator being equal to the iterator returned by End.
    /// \return An iterator referencing the end of the list.
    inline Iterator End() const;

    /// Returns the number of items currently in the list.
    /// \return The number of items in the list.
    inline uint32_t Size() const;

    /// Returns true if there are no items currently in the list.
    inline bool Empty() const;

    /// Empties the list, removing all previously inserted items.
    inline void Clear();

    /// Adds an item to the list. The new item is referenced by pointer and is not copied.
    /// No checking for duplicates is performed.
    /// \param item A pointer to the item to be added to the list.
    inline void Insert(ItemType *const item);

    /// Removes the given item from the list.
    /// \param item A pointer to the item to be removed, which is expected to be present a list node.
    /// \return True if the item was successfully removed, otherwise false.
    inline bool Remove(ItemType *const item);

    /// Returns a pointer to the first item in the list.
    inline ItemType *Front() const;

private:

    IntrusiveList(const IntrusiveList &other);
    IntrusiveList &operator=(const IntrusiveList &other);

    ItemType *mHead;                ///< Pointer to the node at the head of the list.
};


template <class ItemType>
THERON_FORCEINLINE IntrusiveList<ItemType>::IntrusiveList() :
  mHead(0)
{
}


template <class ItemType>
THERON_FORCEINLINE IntrusiveList<ItemType>::~IntrusiveList()
{
    // The list doesn't own the nodes so there's nothing to free.
    Clear();
}


template <class ItemType>
THERON_FORCEINLINE typename IntrusiveList<ItemType>::Iterator IntrusiveList<ItemType>::Begin() const
{
    return Iterator(mHead);
}


template <class ItemType>
THERON_FORCEINLINE typename IntrusiveList<ItemType>::Iterator IntrusiveList<ItemType>::End() const
{
    return Iterator(0);
}


template <class ItemType>
THERON_FORCEINLINE uint32_t IntrusiveList<ItemType>::Size() const
{
    uint32_t count(0);
    ItemType *node(mHead);

    while (node)
    {
        ++count;
        node = node->GetNext();
    }

    return count;
}


template <class ItemType>
THERON_FORCEINLINE bool IntrusiveList<ItemType>::Empty() const
{
    return (mHead == 0);
}


template <class ItemType>
THERON_FORCEINLINE void IntrusiveList<ItemType>::Clear()
{
    // Since the nodes are allocated and owned by the caller, we just forget about them.
    mHead = 0;
}


template <class ItemType>
THERON_FORCEINLINE void IntrusiveList<ItemType>::Insert(ItemType *const item)
{
    item->SetNext(mHead);
    mHead = item;
}


template <class ItemType>
inline bool IntrusiveList<ItemType>::Remove(ItemType *const item)
{
    // Find the node and the one before it, if any.
    ItemType *previous(0);
    ItemType *node(mHead);

    while (node)
    {
        // Note compared by address not value.
        if (node == item)
        {
            break;
        }

        previous = node;
        node = node->GetNext();
    }

    // Node in list?
    if (node)
    {
        // Node at head?
        if (previous == 0)
        {
            mHead = node->GetNext();
        }
        else
        {
            previous->SetNext(node->GetNext());
        }

        return true;
    }

    return false;
}


template <class ItemType>
THERON_FORCEINLINE ItemType *IntrusiveList<ItemType>::Front() const
{
    return mHead;
}


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_CONTAINERS_INTRUSIVELIST_H

