// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_DETAIL_SCHEDULER_ACTORPROCESSOR_H
#define THERON_DETAIL_SCHEDULER_ACTORPROCESSOR_H


#include <Theron/Assert.h>
#include <Theron/BasicTypes.h>
#include <Theron/Defines.h>

#include <Theron/Detail/Messages/IMessage.h>
#include <Theron/Detail/Scheduler/WorkerContext.h>


namespace Theron
{


class Actor;


namespace Detail
{


/**
Processes actors that have received messages.
*/
class ActorProcessor
{
public:

    static void Process(
        WorkerContext *const workerContext,
        Actor *const actor,
        IMessage *const message);
    
private:

    ActorProcessor(const ActorProcessor &other);
    ActorProcessor &operator=(const ActorProcessor &other);
};


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_SCHEDULER_ACTORPROCESSOR_H
