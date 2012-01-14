// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_TESTS_TESTSUITES_ACTORREFTESTSUITE_H
#define THERON_TESTS_TESTSUITES_ACTORREFTESTSUITE_H


#include <Theron/Detail/Directory/Directory.h>
#include <Theron/Detail/Directory/ActorDirectory.h>
#include <Theron/Detail/Threading/Lock.h>

#include <Theron/ActorRef.h>
#include <Theron/Framework.h>

#include "TestFramework/TestSuite.h"


namespace UnitTests
{


class ActorRefTestSuite : public TestFramework::TestSuite
{
public:

    class SimpleActor : public Theron::Actor
    {
    public:

        struct Parameters
        {
        };

        inline SimpleActor()
        {
        }

        inline explicit SimpleActor(const Parameters &/*params*/)
        {
        }
    };

    inline ActorRefTestSuite()
    {
        TESTFRAMEWORK_REGISTER_TESTSUITE(ActorRefTestSuite);
        
        TESTFRAMEWORK_REGISTER_TEST(TestConstruction);
        TESTFRAMEWORK_REGISTER_TEST(TestCopyConstruction);
        TESTFRAMEWORK_REGISTER_TEST(TestAssignment);
        TESTFRAMEWORK_REGISTER_TEST(TestScope);
        TESTFRAMEWORK_REGISTER_TEST(TestTransfer);
    }

    inline virtual ~ActorRefTestSuite()
    {
    }

    inline static void TestConstruction()
    {
        Theron::Framework framework;
        Theron::ActorRef actor(framework.CreateActor<SimpleActor>());
    }
    
    inline static void TestCopyConstruction()
    {
        Theron::Framework framework;
        Theron::ActorRef actor(framework.CreateActor<SimpleActor>());
        
        // Copy construct
        Theron::ActorRef copy(actor);

        Theron::uint32_t count(0);

        {
            Theron::Detail::Lock lock(Theron::Detail::Directory::GetMutex());
            count = Theron::Detail::ActorDirectory::Instance().Count();
        }

        Check(count == 1, "Copy construction failed");
    }
    
    inline static void TestAssignment()
    {
        Theron::Framework framework;
        Theron::ActorRef actorOne(framework.CreateActor<SimpleActor>());
        Theron::ActorRef actorTwo(framework.CreateActor<SimpleActor>());

        // Assign
        actorOne = actorTwo;

        // Wait for actor one to be destroyed.
        Theron::uint32_t numEntities(2);
        while (numEntities != 1)
        {
            Check(numEntities == 2, "Copy construction failed");

            Theron::Detail::Lock lock(Theron::Detail::Directory::GetMutex());
            numEntities = Theron::Detail::ActorDirectory::Instance().Count();
        }
    }
    
    inline static void TestScope()
    {
        Theron::Framework framework;
        Theron::uint32_t numEntities(0);
        
        {
            Theron::ActorRef actorOne(framework.CreateActor<SimpleActor>());
            (void) actorOne;

            numEntities = 0;
            while (numEntities != 1)
            {
                Check(numEntities == 0, "Incorrect entity count");

                Theron::Detail::Lock lock(Theron::Detail::Directory::GetMutex());
                numEntities = Theron::Detail::ActorDirectory::Instance().Count();
            }
            
            {
                Theron::ActorRef actorTwo(framework.CreateActor<SimpleActor>());
                (void) actorTwo;

                numEntities = 1;
                while (numEntities != 2)
                {
                    Check(numEntities == 1, "Incorrect entity count");

                    Theron::Detail::Lock lock(Theron::Detail::Directory::GetMutex());
                    numEntities = Theron::Detail::ActorDirectory::Instance().Count();
                }
            }

            numEntities = 2;
            while (numEntities != 1)
            {
                Check(numEntities == 2, "Incorrect entity count");

                Theron::Detail::Lock lock(Theron::Detail::Directory::GetMutex());
                numEntities = Theron::Detail::ActorDirectory::Instance().Count();
            }
        }

        numEntities = 1;
        while (numEntities != 0)
        {
            Check(numEntities == 1, "Incorrect entity count");

            Theron::Detail::Lock lock(Theron::Detail::Directory::GetMutex());
            numEntities = Theron::Detail::ActorDirectory::Instance().Count();
        }
    }
    
    inline static void TestTransfer()
    {
        Theron::Framework framework;
        Theron::uint32_t numEntities(0);

        Theron::ActorRef actorOne(framework.CreateActor<SimpleActor>());
        
        {
            Theron::ActorRef actorTwo(framework.CreateActor<SimpleActor>());

            numEntities = 1;
            while (numEntities != 2)
            {
                Check(numEntities == 1, "Incorrect entity count");

                Theron::Detail::Lock lock(Theron::Detail::Directory::GetMutex());
                numEntities = Theron::Detail::ActorDirectory::Instance().Count();
            }

            actorOne = actorTwo;
        }

        numEntities = 2;
        while (numEntities != 1)
        {
            Check(numEntities == 2, "Incorrect entity count");

            Theron::Detail::Lock lock(Theron::Detail::Directory::GetMutex());
            numEntities = Theron::Detail::ActorDirectory::Instance().Count();
        }
    }
};


} // namespace UnitTests


#endif // THERON_TESTS_TESTSUITES_ACTORREFTESTSUITE_H
