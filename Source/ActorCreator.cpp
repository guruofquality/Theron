// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#include <Theron/Detail/Core/ActorCreator.h>


namespace Theron
{
namespace Detail
{


Mutex ActorCreator::smMutex;
Address ActorCreator::smAddress;
ActorCore *ActorCreator::smCoreAddress = 0;


} // namespace Detail
} // namespace Theron

