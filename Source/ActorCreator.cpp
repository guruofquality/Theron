// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#include <Theron/Detail/Core/ActorCreator.h>
#include <Theron/Detail/Threading/Lock.h>


namespace Theron
{
namespace Detail
{


Mutex ActorCreator::smMutex;
ActorCreator::Entry *ActorCreator::smHead = 0;


void ActorCreator::Register(Entry *const entry)
{
    Lock lock(smMutex);

    // Singly-linked list insert at head.
    entry->mNext = smHead;
    smHead = entry;
}


void ActorCreator::Deregister(Entry *const entry)
{
    Lock lock(smMutex);

    // Singly-linked list remove.
    Entry *previous(0);
    Entry *node(smHead);

    while (node && node != entry)
    {
        previous = node;
        node = node->mNext;
    }

    if (node)
    {
        if (previous)
        {
            previous->mNext = node->mNext;
        }
        else
        {
            smHead = node->mNext;
        }
    }
}


ActorCreator::Entry *ActorCreator::Get(void *const location)
{
    Lock lock(smMutex);

    // Singly-linked list search.
    Entry *node(smHead);

    while (node && node->mLocation != location)
    {
        node = node->mNext;
    }

    return node;
}


} // namespace Detail
} // namespace Theron

