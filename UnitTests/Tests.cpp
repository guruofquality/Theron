// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#include "TestSuites/DefaultAllocatorTestSuite.h"
#include "TestSuites/PoolTestSuite.h"
#include "TestSuites/MessageCacheTestSuite.h"
#include "TestSuites/ListTestSuite.h"
#include "TestSuites/MessageTestSuite.h"
#include "TestSuites/ReceiverTestSuite.h"
#include "TestSuites/ThreadCollectionTestSuite.h"
#include "TestSuites/ActorRefTestSuite.h"
#include "TestSuites/ActorTestSuite.h"
#include "TestSuites/FrameworkTestSuite.h"
#include "TestSuites/FeatureTestSuite.h"


namespace UnitTests
{


/// Instantiations of the unit test suites.
DefaultAllocatorTestSuite defaultAllocatorTestSuite;
PoolTestSuite poolTestSuite;
MessageCacheTestSuite messageCacheTestSuite;
ListTestSuite listTestSuite;
MessageTestSuite messageTestSuite;
ReceiverTestSuite receiverTestSuite;
ThreadCollectionTestSuite threadCollectionTestSuite;
ActorRefTestSuite actorRefTestSuite;
ActorTestSuite actorTestSuite;
FrameworkTestSuite frameworkTestSuite;
FeatureTestSuite featureTestSuite;


} // namespace UnitTests


