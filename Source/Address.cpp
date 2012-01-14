// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#include <Theron/Detail/BasicTypes.h>
#include <Theron/Detail/Threading/Mutex.h>

#include <Theron/Address.h>


namespace Theron
{


Detail::Mutex Address::smMutex;
uint32_t Address::smNextValue = 1;


} // namespace Theron


