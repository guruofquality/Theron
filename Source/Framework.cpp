// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#include <Theron/Detail/MessageCache/MessageCache.h>

#include <Theron/AllocatorManager.h>
#include <Theron/Framework.h>


namespace Theron
{


Framework::Framework() :
  mThreadPool(),
  mFallbackMessageHandler(0),
  mDefaultFallbackHandler()
{
    Initialize(2);
}


Framework::Framework(const uint32_t numThreads) :
  mThreadPool(),
  mFallbackMessageHandler(0),
  mDefaultFallbackHandler()
{
    Initialize(numThreads);
}


Framework::~Framework()
{
    mThreadPool.Stop();

    // Dereference the global free list to ensure it's destroyed.
    Detail::MessageCache::Instance().Dereference();

    // Free the fallback handler object, if one is set.
    if (mFallbackMessageHandler)
    {
        AllocatorManager::Instance().GetAllocator()->Free(mFallbackMessageHandler);
    }
}


} // namespace Theron

