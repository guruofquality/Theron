// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#include <Theron/Actor.h>
#include <Theron/Framework.h>


namespace Theron
{


uint32_t *Actor::GetPulseCounterAddress() const
{
    return GetFramework().GetPulseCounterAddress();
}


} // namespace Theron


