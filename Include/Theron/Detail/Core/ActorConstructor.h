// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_DETAIL_CORE_ACTORCONSTRUCTOR_H
#define THERON_DETAIL_CORE_ACTORCONSTRUCTOR_H


#include <Theron/Detail/Debug/Assert.h>

#include <Theron/Defines.h>


namespace Theron
{
namespace Detail
{


/// Helper class template that constructs actors.
/// The point of this class is really to allow reuse of the same creation code
/// in ActorCreator. The template is partially specialized as a trick to avoid
/// referencing a non-existant Parameters subclass on actor types that don't
/// expose one.
template <class TActor, bool withParameters>
class ActorConstructor
{
public:

    typedef TActor ActorType;

    THERON_FORCEINLINE ActorConstructor()
    {
    }

    /// Policy operator.
    THERON_FORCEINLINE ActorType *operator()(void *const memory) const
    {
        THERON_ASSERT(memory);
        return new (memory) ActorType();
    }

private:

    ActorConstructor(const ActorConstructor &other);
    ActorConstructor &operator=(const ActorConstructor &other);
};


/// Specialization for true, where the actor class has a Parameters subtype.
template <class TActor>
class ActorConstructor<TActor, true>
{
public:

    typedef TActor ActorType;
    typedef typename ActorType::Parameters Parameters;

    THERON_FORCEINLINE explicit ActorConstructor(const Parameters &params) : mParams(&params)
    {
    }

    /// Policy operator.
    THERON_FORCEINLINE ActorType *operator()(void *const memory) const
    {
        THERON_ASSERT(memory);
        return new (memory) ActorType(*mParams);
    }

private:

    ActorConstructor(const ActorConstructor &other);
    ActorConstructor &operator=(const ActorConstructor &other);

    const Parameters *const mParams;
};


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_CORE_ACTORCONSTRUCTOR_H

