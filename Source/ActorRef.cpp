// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#include <Theron/ActorRef.h>
#include <Theron/Framework.h>


namespace Theron
{


uint32_t *ActorRef::GetPulseCounterAddress() const
{
    return mActor->GetFramework().GetPulseCounterAddress();
}


} // namespace Theron


