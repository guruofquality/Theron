// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#include <Theron/AllocatorManager.h>


namespace Theron
{


DefaultAllocator AllocatorManager::smDefaultAllocator;
AllocatorManager::CacheType AllocatorManager::smCache(&smDefaultAllocator);


} // namespace Theron


