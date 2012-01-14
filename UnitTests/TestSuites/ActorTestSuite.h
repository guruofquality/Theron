// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_TESTS_TESTSUITES_ACTORTESTSUITE_H
#define THERON_TESTS_TESTSUITES_ACTORTESTSUITE_H


#include <Theron/Detail/BasicTypes.h>

#include <Theron/Detail/Threading/Mutex.h>

#include <Theron/Framework.h>
#include <Theron/Actor.h>
#include <Theron/Receiver.h>

#include "TestFramework/TestSuite.h"


namespace UnitTests
{


class ActorTestSuite : public TestFramework::TestSuite
{
public:

    class ActorHandlerConstructor : public Theron::Actor
    {
    public:

        inline ActorHandlerConstructor()
        {
            RegisterHandler(this, &ActorHandlerConstructor::Handler);
        }

    private:

        inline void Handler(const int &message, const Theron::Address from)
        {
            Send(message, from);
        }
    };

    class OneHandlerActor : public Theron::Actor
    {
    public:

        struct Message
        {
        };

        inline OneHandlerActor()
        {
            RegisterHandler(this, &OneHandlerActor::Handler);
        }

    private:

        inline void Handler(const Message &/*message*/, const Theron::Address /*from*/)
        {
        }
    };

    class TwoHandlerActor : public Theron::Actor
    {
    public:

        struct MessageOne
        {
        };

        struct MessageTwo
        {
        };

        inline TwoHandlerActor()
        {
            RegisterHandler(this, &TwoHandlerActor::HandlerOne);
            RegisterHandler(this, &TwoHandlerActor::HandlerTwo);
        }

    private:

        inline void HandlerOne(const MessageOne &/*message*/, const Theron::Address /*from*/)
        {
        }

        inline void HandlerTwo(const MessageTwo &/*message*/, const Theron::Address /*from*/)
        {
        }
    };

    class ResponderActor : public Theron::Actor
    {
    public:

        inline ResponderActor()
        {
            RegisterHandler(this, &ResponderActor::Handler);
        }

    private:

        inline void Handler(const Theron::uint32_t &value, const Theron::Address from)
        {
            Send(value, from);
        }
    };

    class Listener
    {
    public:

        inline Listener() :
          mValue(0),
          mFrom()
        {
        }
        
        inline explicit Listener(const Theron::uint32_t value) :
          mValue(value),
          mFrom()
        {
        }
       
        inline Theron::uint32_t GetValue()
        {
            Theron::uint32_t result(0);
            
            mMutex.Lock();
            result = mValue;
            mMutex.Unlock();
            
            return result;
        }
        
        inline Theron::Address From()
        {
            mMutex.Lock();
            Theron::Address result(mFrom);
            mMutex.Unlock();
            
            return result;
        }
        
        inline void Handle(const Theron::uint32_t &value, const Theron::Address from)
        {
            mMutex.Lock();
            mValue = value;
            mFrom = from;
            mMutex.Unlock();
        }
        
    private:
        
        Theron::Detail::Mutex mMutex;
        Theron::uint32_t mValue;
        Theron::Address mFrom;
    };

    inline ActorTestSuite()
    {
        TESTFRAMEWORK_REGISTER_TESTSUITE(ActorTestSuite);

        TESTFRAMEWORK_REGISTER_TEST(TestCreateActorWithOneHandler);
        TESTFRAMEWORK_REGISTER_TEST(TestCreateActorWithTwoHandlers);
        TESTFRAMEWORK_REGISTER_TEST(TestExecuteOneHandler);
        TESTFRAMEWORK_REGISTER_TEST(TestSendMessage);
        TESTFRAMEWORK_REGISTER_TEST(TestSwamping);
        TESTFRAMEWORK_REGISTER_TEST(TestRegisterInConstructor);
    }

    inline virtual ~ActorTestSuite()
    {
    }

    inline static void TestCreateActorWithOneHandler()
    {
        Theron::Framework framework;
        Theron::ActorRef actor(framework.CreateActor<OneHandlerActor>());
    }

    inline static void TestCreateActorWithTwoHandlers()
    {
        Theron::Framework framework;
        Theron::ActorRef actor(framework.CreateActor<TwoHandlerActor>());
    }

    inline static void TestExecuteOneHandler()
    {
        Theron::Framework framework;
        Theron::ActorRef actor(framework.CreateActor<OneHandlerActor>());

        Theron::Address here;
        OneHandlerActor::Message message;
        actor.Push(message, here);
    }

    inline static void TestSendMessage()
    {
        // Create a responder actor.
        Theron::Framework framework;
        Theron::ActorRef responder(framework.CreateActor<ResponderActor>());

        // Create a listener to collect messages.
        Listener listener(0);
        
        // Create a receiver and register a handler method on the listener.
        Theron::Receiver receiver;
        receiver.RegisterHandler(&listener, &Listener::Handle);

        // Push a test message to the responder actor, using the
        // address of the receiver as the from address.
        Theron::uint32_t value(1);
        responder.Push(value, receiver.GetAddress());

        // When the ResponderActor receives a message, it sends a
        // message back to the sender in aknowledgement, using its
        // internal Send() method. The return message is addressed
        // to the receiver. On receiving it, the receiver will execute
        // the registered handler method of the listener, updating
        // the listener with the contents of the message.
        // We wait here to be alerted that the message has arrived
        // and the handler has been executed.
        receiver.Wait();

        // Check the message the listener received was from the ResponderActor.
        Check(listener.From() == responder.GetAddress(), "Send failed");
    }

    /// Tests that an actor swamped with messages will process all of them.
    inline static void TestSwamping()
    {
        // Create a responder actor.
        Theron::Framework framework;
        Theron::ActorRef responder(framework.CreateActor<ResponderActor>());

        Theron::Receiver receiver;

        // Push 5 messages to the responder actor, using the
        // address of the receiver as the from address.
        // After each send, wait for a return message.
        Theron::uint32_t value(1);

        const Theron::uint32_t NUM_MESSAGES = 1000;
        for (Theron::uint32_t count = 0; count < NUM_MESSAGES; ++count)
        {
            responder.Push(value, receiver.GetAddress());
        }

        for (Theron::uint32_t count = 0; count < NUM_MESSAGES; ++count)
        {
            receiver.Wait();
        }
    }

    // Tests that it's possible to register handlers in an actor constructor.
    inline static void TestRegisterInConstructor()
    {
        Theron::Framework framework;

        Theron::ActorRef actor(framework.CreateActor<ActorHandlerConstructor>());

        Theron::Receiver receiver;
        actor.Push(5, receiver.GetAddress());

        receiver.Wait();
    }
};


} // namespace UnitTests


#endif // THERON_TESTS_TESTSUITES_ACTORTESTSUITE_H


