// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_DETAIL_CORE_ACTORDESTROYER_H
#define THERON_DETAIL_CORE_ACTORDESTROYER_H


namespace Theron
{
namespace Detail
{


class ActorCore;


class ActorDestroyer
{
public:

    static void DestroyActor(ActorCore *const actorCore);
};


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_CORE_ACTORDESTROYER_H

