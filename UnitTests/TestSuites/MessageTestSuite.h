// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_TESTS_TESTSUITES_MESSAGETESTSUITE_H
#define THERON_TESTS_TESTSUITES_MESSAGETESTSUITE_H


#include <Theron/Detail/Messages/IMessage.h>
#include <Theron/Detail/Messages/Message.h>

#include <Theron/Address.h>
#include <Theron/AllocatorManager.h>
#include <Theron/Register.h>

#include "TestFramework/TestSuite.h"


namespace UnitTests
{


struct MessageValue
{
    inline MessageValue() : a(0), b(0)
    {
    }

    int a;
    int b;
};


struct NamedMessageValue
{
    inline NamedMessageValue() : a(0), b(0)
    {
    }

    int a;
    int b;
};


} // namespace UnitTests


THERON_REGISTER_MESSAGE(UnitTests::NamedMessageValue);


namespace UnitTests
{


class MessageTestSuite : public TestFramework::TestSuite
{
public:

    inline MessageTestSuite()
    {
        TESTFRAMEWORK_REGISTER_TESTSUITE(MessageTestSuite);
        
        TESTFRAMEWORK_REGISTER_TEST(TestGetSize);
        TESTFRAMEWORK_REGISTER_TEST(TestGetAlignment);
        TESTFRAMEWORK_REGISTER_TEST(TestConstruction);
        TESTFRAMEWORK_REGISTER_TEST(TestValue);
        TESTFRAMEWORK_REGISTER_TEST(TestIMessage);
        TESTFRAMEWORK_REGISTER_TEST(TestFrom);
        TESTFRAMEWORK_REGISTER_TEST(TestGetBlock);
        TESTFRAMEWORK_REGISTER_TEST(TestGetBlockSize);
        TESTFRAMEWORK_REGISTER_TEST(TestTypeNameUnregistered);
        TESTFRAMEWORK_REGISTER_TEST(TestTypeNameRegistered);
    }

    inline virtual ~MessageTestSuite()
    {
    }

    inline static void TestGetSize()
    {
        typedef Theron::Detail::Message<MessageValue> MessageType;
        const Theron::uint32_t messageSize(MessageType::GetSize());
        Check(messageSize >= 4, "Message::GetSize() returned invalid size");
    }

    inline static void TestGetAlignment()
    {
        typedef Theron::Detail::Message<MessageValue> MessageType;
        const Theron::uint32_t messageAlignment(MessageType::GetAlignment());
        Check(messageAlignment >= 4, "Message::TestGetAlignment() returned alignment less than 4");
        Check((messageAlignment % 4) == 0, "Message::TestGetAlignment() returned non-power-of-two");
    }

    inline static void TestConstruction()
    {
        typedef Theron::Detail::Message<MessageValue> MessageType;

        Theron::Address here;
        Theron::IAllocator &allocator(*Theron::AllocatorManager::Instance().GetAllocator());

        const Theron::uint32_t messageSize(MessageType::GetSize());
        const Theron::uint32_t messageAlignment(MessageType::GetAlignment());

        void *const memory = allocator.AllocateAligned(messageSize, messageAlignment);

        MessageValue value;
        MessageType *const message = MessageType::Initialize(memory, value, here);

        Check(message != 0, "Failed to initialize message");

        allocator.Free(memory);
    }

    inline static void TestValue()
    {
        typedef Theron::Detail::Message<MessageValue> MessageType;

        Theron::Address here;
        Theron::IAllocator &allocator(*Theron::AllocatorManager::Instance().GetAllocator());

        const Theron::uint32_t messageSize(MessageType::GetSize());
        const Theron::uint32_t messageAlignment(MessageType::GetAlignment());

        void *const memory = allocator.AllocateAligned(messageSize, messageAlignment);

        MessageValue value;
        value.a = 3;
        value.b = 7;

        MessageType *const message = MessageType::Initialize(memory, value, here);

        Check(message->Value().a == 3, "Message value incorrect");
        Check(message->Value().b == 7, "Message value incorrect");

        allocator.Free(memory);
    }

    inline static void TestIMessage()
    {
        typedef Theron::Detail::Message<MessageValue> MessageType;

        Theron::Address here;
        Theron::IAllocator &allocator(*Theron::AllocatorManager::Instance().GetAllocator());

        const Theron::uint32_t messageSize(MessageType::GetSize());
        const Theron::uint32_t messageAlignment(MessageType::GetAlignment());

        void *const memory = allocator.AllocateAligned(messageSize, messageAlignment);

        MessageValue value;
        MessageType *const message = MessageType::Initialize(memory, value, here);

        Check(dynamic_cast<Theron::Detail::IMessage *>(message) != 0, "Failed to cast message to IMessage");

        allocator.Free(memory);
    }

    inline static void TestFrom()
    {
        typedef Theron::Detail::Message<MessageValue> MessageType;

        Theron::Address here;
        Theron::IAllocator &allocator(*Theron::AllocatorManager::Instance().GetAllocator());

        const Theron::uint32_t messageSize(MessageType::GetSize());
        const Theron::uint32_t messageAlignment(MessageType::GetAlignment());

        void *const memory = allocator.AllocateAligned(messageSize, messageAlignment);

        MessageValue value;
        Theron::Detail::IMessage *const message = MessageType::Initialize(memory, value, here);

        Check(message->From() == here, "Message from address incorrect");

        allocator.Free(memory);
    }

    inline static void TestGetBlock()
    {
        typedef Theron::Detail::Message<MessageValue> MessageType;

        Theron::Address here;
        Theron::IAllocator &allocator(*Theron::AllocatorManager::Instance().GetAllocator());

        const Theron::uint32_t messageSize(MessageType::GetSize());
        const Theron::uint32_t messageAlignment(MessageType::GetAlignment());

        void *const memory = allocator.AllocateAligned(messageSize, messageAlignment);

        MessageValue value;
        Theron::Detail::IMessage *const message = MessageType::Initialize(memory, value, here);

        Check(message->GetBlock() == memory, "Message block address incorrect");

        allocator.Free(message->GetBlock());
    }

    inline static void TestGetBlockSize()
    {
        typedef Theron::Detail::Message<MessageValue> MessageType;

        Theron::Address here;
        Theron::IAllocator &allocator(*Theron::AllocatorManager::Instance().GetAllocator());

        const Theron::uint32_t messageSize(MessageType::GetSize());
        const Theron::uint32_t messageAlignment(MessageType::GetAlignment());

        void *const memory = allocator.AllocateAligned(messageSize, messageAlignment);

        MessageValue value;
        Theron::Detail::IMessage *const message = MessageType::Initialize(memory, value, here);

        Check(message->GetBlockSize() == messageSize, "Message block size incorrect");

        allocator.Free(memory);
    }

    inline static void TestTypeNameUnregistered()
    {
        typedef Theron::Detail::Message<MessageValue> MessageType;

        Theron::Address here;
        Theron::IAllocator &allocator(*Theron::AllocatorManager::Instance().GetAllocator());

        const Theron::uint32_t messageSize(MessageType::GetSize());
        const Theron::uint32_t messageAlignment(MessageType::GetAlignment());

        void *const memory = allocator.AllocateAligned(messageSize, messageAlignment);

        MessageValue value;
        Theron::Detail::IMessage *const message = MessageType::Initialize(memory, value, here);

        Check(message->TypeName() == 0, "Unregistered message type has non-zero type name");

        allocator.Free(memory);
    }

    inline static void TestTypeNameRegistered()
    {
        typedef Theron::Detail::Message<NamedMessageValue> MessageType;

        Theron::Address here;
        Theron::IAllocator &allocator(*Theron::AllocatorManager::Instance().GetAllocator());

        const Theron::uint32_t messageSize(MessageType::GetSize());
        const Theron::uint32_t messageAlignment(MessageType::GetAlignment());

        void *const memory = allocator.AllocateAligned(messageSize, messageAlignment);

        NamedMessageValue value;
        Theron::Detail::IMessage *const message = MessageType::Initialize(memory, value, here);

        Check(message->TypeName() != 0, "Registered message type has zero type name");

        allocator.Free(memory);
    }
};


} // namespace UnitTests


#endif // THERON_TESTS_TESTSUITES_MESSAGETESTSUITE_H

