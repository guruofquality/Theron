// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_TESTS_TESTSUITES_RECEIVERTESTSUITE_H
#define THERON_TESTS_TESTSUITES_RECEIVERTESTSUITE_H


#include <new>

#include <Theron/Detail/BasicTypes.h>

#include <Theron/Detail/Messages/IMessage.h>
#include <Theron/Detail/Messages/Message.h>

#include <Theron/Address.h>
#include <Theron/AllocatorManager.h>
#include <Theron/Framework.h>
#include <Theron/Receiver.h>

#include "TestFramework/TestSuite.h"


namespace UnitTests
{


class ReceiverTestSuite : public TestFramework::TestSuite
{
public:

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

    class MockMessage
    {
    public:

        inline explicit MockMessage(const Theron::uint32_t value) : mValue(value)
        {
        }

        inline const Theron::uint32_t &Value() const
        {
            return mValue;
        }

        inline Theron::uint32_t &Value()
        {
            return mValue;
        }

        inline bool operator==(const MockMessage &other) const
        {
            return (mValue == other.mValue);
        }

    private:

        Theron::uint32_t mValue;
    };

    class Listener
    {
    public:

        inline Listener() :
          mValue(0),
          mFrom()
        {
        }
        
        inline explicit Listener(const MockMessage value) :
          mValue(value),
          mFrom()
        {
        }
       
        inline MockMessage &Value()
        {
            return mValue;
        }
        
        inline Theron::Address &From()
        {
            return mFrom;
        }
        
        inline void Handle(const MockMessage &message, const Theron::Address from)
        {
            mValue = message;
            mFrom = from;
        }
        
    private:
        
        MockMessage mValue;
        Theron::Address mFrom;
    };

    inline ReceiverTestSuite()
    {
        TESTFRAMEWORK_REGISTER_TESTSUITE(ReceiverTestSuite);
        
        TESTFRAMEWORK_REGISTER_TEST(TestConstruction);
        TESTFRAMEWORK_REGISTER_TEST(TestRegistration);
        TESTFRAMEWORK_REGISTER_TEST(TestDeregistration);
        TESTFRAMEWORK_REGISTER_TEST(TestPush);
        TESTFRAMEWORK_REGISTER_TEST(TestPushWithoutRegistration);
        TESTFRAMEWORK_REGISTER_TEST(TestPushAfterDeregistration);
        TESTFRAMEWORK_REGISTER_TEST(TestWait);
        TESTFRAMEWORK_REGISTER_TEST(TestMultipleWaits);
        TESTFRAMEWORK_REGISTER_TEST(TestWaitFencing);
        TESTFRAMEWORK_REGISTER_TEST(TestCount);
        TESTFRAMEWORK_REGISTER_TEST(TestReset);
    }

    inline virtual ~ReceiverTestSuite()
    {
    }

    inline static void TestConstruction()
    {
        Theron::Receiver receiver;
    }

    inline static void TestRegistration()
    {
        Listener listener;
        Theron::Receiver receiver;
        receiver.RegisterHandler(&listener, &Listener::Handle);
    }

    inline static void TestDeregistration()
    {
        Listener listener;
        Theron::Receiver receiver;
        receiver.RegisterHandler(&listener, &Listener::Handle);
        receiver.DeregisterHandler(&listener, &Listener::Handle);
    }

    inline static void TestPush()
    {
        Listener listener(MockMessage(1));
        Theron::Receiver receiver;
        receiver.RegisterHandler(&listener, &Listener::Handle);

        const MockMessage value(2);
        Theron::Address fromAddress;

        Theron::Detail::IMessage *const message = CreateMessage(value, fromAddress);
        Check(message != 0, "Failed to construct message");

        receiver.Push(message);
        Check(listener.Value() == value, "Registered handler not executed correctly");
        Check(listener.From() == fromAddress, "Registered handler not executed correctly");

        receiver.DeregisterHandler(&listener, &Listener::Handle);
    }

    inline static void TestPushWithoutRegistration()
    {
        typedef Theron::Detail::Message<MockMessage> MessageType;
        
        Listener listener(MockMessage(1));
        Theron::Receiver receiver;

        const MockMessage value(2);
        Theron::Address fromAddress;

        Theron::Detail::IMessage *const message = CreateMessage(value, fromAddress);
        Check(message != 0, "Failed to construct message");

        receiver.Push(message);
        Check(listener.Value() == MockMessage(1), "Unregistered handler executed");
    }

    inline static void TestPushAfterDeregistration()
    {
        typedef Theron::Detail::Message<MockMessage> MessageType;
        
        Listener listener(MockMessage(1));
        Theron::Receiver receiver;

        receiver.RegisterHandler(&listener, &Listener::Handle);
        receiver.DeregisterHandler(&listener, &Listener::Handle);

        const MockMessage value(2);
        Theron::Address fromAddress;

        Theron::Detail::IMessage *const message = CreateMessage(value, fromAddress);
        Check(message != 0, "Failed to construct message");

        receiver.Push(message);
        Check(listener.Value() == MockMessage(1), "Handler executed after deregistration");
    }

    inline static void TestWait()
    {
        // Create a responder actor.
        Theron::Framework framework;
        Theron::ActorRef responder(framework.CreateActor<ResponderActor>());

        Theron::Receiver receiver;

        // Push a test message to the responder actor, using the
        // address of the receiver as the from address.
        Theron::uint32_t value(1);
        responder.Push(value, receiver.GetAddress());

        // Wait for the return message to be received.
        receiver.Wait();
    }

    inline static void TestMultipleWaits()
    {
        // Create a responder actor.
        Theron::Framework framework;
        Theron::ActorRef responder(framework.CreateActor<ResponderActor>());

        Theron::Receiver receiver;

        // Push 5 messages to the responder actor, using the
        // address of the receiver as the from address.
        Theron::uint32_t value(1);
        responder.Push(value, receiver.GetAddress());
        responder.Push(value, receiver.GetAddress());
        responder.Push(value, receiver.GetAddress());
        responder.Push(value, receiver.GetAddress());
        responder.Push(value, receiver.GetAddress());

        // Wait for 5 return messages to be received.
        receiver.Wait();
        receiver.Wait();
        receiver.Wait();
        receiver.Wait();
        receiver.Wait();
    }
    
    inline static void TestWaitFencing()
    {
        // Create a responder actor.
        Theron::Framework framework;
        Theron::ActorRef responder(framework.CreateActor<ResponderActor>());

        Theron::Receiver receiver;

        // Push 5 messages to the responder actor, using the
        // address of the receiver as the from address.
        // After each send, wait for a return message.
        Theron::uint32_t value(1);

        responder.Push(value, receiver.GetAddress());
        receiver.Wait();
        
        responder.Push(value, receiver.GetAddress());
        receiver.Wait();

        responder.Push(value, receiver.GetAddress());
        receiver.Wait();

        responder.Push(value, receiver.GetAddress());
        receiver.Wait();

        responder.Push(value, receiver.GetAddress());
        receiver.Wait();
    }

    inline static void TestCount()
    {
        // Create a responder actor.
        Theron::Framework framework;
        Theron::ActorRef responder(framework.CreateActor<ResponderActor>());

        Theron::Receiver receiver;

        // Check initial count value.
        Check(receiver.Count() == 0, "Receiver::Count failed");

        // Push a test message to the responder actor, using the
        // address of the receiver as the from address.
        Theron::uint32_t value(1);
        responder.Push(value, receiver.GetAddress());

        // Busy-wait until the count becomes non-zero
        Theron::uint32_t count = 0;
        while (count == 0)
        {
            count = receiver.Count();
        }

        // Check new count value.
        Check(receiver.Count() == 1, "Receiver::Count failed");

        // Wait for the received message.
        receiver.Wait();

        // Check count value after successful wait.
        Check(receiver.Count() == 0, "Receiver::Count failed");
    }

    inline static void TestReset()
    {
        // Create a responder actor.
        Theron::Framework framework;
        Theron::ActorRef responder(framework.CreateActor<ResponderActor>());

        Theron::Receiver receiver;

        // Check initial count value.
        Check(receiver.Count() == 0, "Receiver::Count failed");

        // Reset the count to test handling of trivial resets.
        receiver.Reset();

        // Check reset count value.
        Check(receiver.Count() == 0, "Receiver::Count failed");

        // Push a test message to the responder actor, using the
        // address of the receiver as the from address.
        Theron::uint32_t value(1);
        responder.Push(value, receiver.GetAddress());

        // Busy-wait until the count becomes non-zero
        Theron::uint32_t count = 0;
        while (count == 0)
        {
            count = receiver.Count();
        }

        // Check new count value.
        Check(receiver.Count() == 1, "Receiver::Count failed");

        // Reset the count.
        receiver.Reset();

        // Check count value after reset
        Check(receiver.Count() == 0, "Receiver::Count failed");

        // Push a second test message.
        responder.Push(value, receiver.GetAddress());

        // Wait for the received message to check waiting still works.
        receiver.Wait();
    }

private:

    template <class ValueType>
    inline static Theron::Detail::IMessage *CreateMessage(const ValueType &value, const Theron::Address &from)
    {
        typedef Theron::Detail::Message<ValueType> MessageType;

        Theron::IAllocator &allocator(*Theron::AllocatorManager::Instance().GetAllocator());

        const Theron::uint32_t messageSize(MessageType::GetSize());
        const Theron::uint32_t messageAlignment(MessageType::GetAlignment());

        void *const memory = allocator.AllocateAligned(messageSize, messageAlignment);
        Theron::Detail::IMessage *const message = MessageType::Initialize(memory, value, from);

        return message;
    }

    inline static void DestroyMessage(Theron::Detail::IMessage *const message)
    {
        Theron::IAllocator &allocator(*Theron::AllocatorManager::Instance().GetAllocator());
        allocator.Free(message->GetBlock());
    }
};


} // namespace UnitTests


#endif // THERON_TESTS_TESTSUITES_RECEIVERTESTSUITE_H

