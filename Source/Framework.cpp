// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


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

    // Free the fallback handler object, if one is set.
    if (mFallbackMessageHandler)
    {
        AllocatorManager::Instance().GetAllocator()->Free(mFallbackMessageHandler);
    }
}


} // namespace Theron

