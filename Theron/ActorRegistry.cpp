// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#include <Theron/Detail/Legacy/ActorRegistry.h>


namespace Theron
{
namespace Detail
{


SpinLock ActorRegistry::smSpinLock;
ActorRegistry::Entry *ActorRegistry::smHead = 0;


void ActorRegistry::Register(Entry *const entry)
{
    smSpinLock.Lock();

    // Singly-linked list insert at head.
    entry->mNext = smHead;
    smHead = entry;

    smSpinLock.Unlock();
}


void ActorRegistry::Deregister(Entry *const entry)
{
    smSpinLock.Lock();

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

    smSpinLock.Unlock();
}


ActorRegistry::Entry *ActorRegistry::Lookup(Actor *const actor)
{
    // Singly-linked list search.
    Entry *node(0);

    smSpinLock.Lock();

    // Singly-linked list search.
    node = smHead;
    while (node && node->mActor != actor)
    {
        node = node->mNext;
    }

    smSpinLock.Unlock();

    return node;
}


} // namespace Detail
} // namespace Theron

