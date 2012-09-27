// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_TESTS_TESTSUITES_LEGACYTESTSUITE_H
#define THERON_TESTS_TESTSUITES_LEGACYTESTSUITE_H

#include <vector>
#include <string>
#include <queue>

#include <Theron/Theron.h>

#include "TestFramework/TestSuite.h"


namespace Tests
{


class LegacyTestSuite : public TestFramework::TestSuite
{
public:

    class Trivial : public Theron::Actor
    {
    };

    template <class MessageType>
    class Replier : public Theron::Actor
    {
    public:

        inline Replier()
        {
            RegisterHandler(this, &Replier::Handler);
        }

    private:

        inline void Handler(const MessageType &message, const Theron::Address from)
        {
            Send(message, from);
        }
    };

    template <class MessageType>
    class DefaultReplier : public Theron::Actor
    {
    public:

        inline DefaultReplier()
        {
            RegisterHandler(this, &DefaultReplier::Handler);
            SetDefaultHandler(this, &DefaultReplier::DefaultHandler);
        }

    private:

        inline void Handler(const MessageType &message, const Theron::Address from)
        {
            Send(message, from);
        }

        inline void DefaultHandler(const Theron::Address from)
        {
            std::string hello("hello");
            Send(hello, from);
        }
    };

    class StringReplier : public Replier<const char *>
    {
    };

    class Signaler : public Theron::Actor
    {
    public:

        inline Signaler()
        {
            RegisterHandler(this, &Signaler::Signal);
        }

    private:

        inline void Signal(const Theron::Address &address, const Theron::Address from)
        {
            // Send the 'from' address to the address received in the message.
            Send(from, address);
        }
    };

    class Poker : public Theron::Actor
    {
    public:

        inline Poker()
        {
            RegisterHandler(this, &Poker::Poke);
        }

    private:

        inline void Poke(const Theron::ActorRef &actor, const Theron::Address from)
        {
            // Poke the actor referenced by the message, sending it the from address.
            Send(from, actor.GetAddress());
        }
    };

    class Switcher : public Theron::Actor
    {
    public:

        inline Switcher()
        {
            RegisterHandler(this, &Switcher::SayHello);
        }

    private:

        inline void SayHello(const std::string &/*message*/, const Theron::Address from)
        {
            DeregisterHandler(this, &Switcher::SayHello);
            RegisterHandler(this, &Switcher::SayGoodbye);
            Send(std::string("hello"), from);
        }

        inline void SayGoodbye(const std::string &/*message*/, const Theron::Address from)
        {
            DeregisterHandler(this, &Switcher::SayGoodbye);
            RegisterHandler(this, &Switcher::SayHello);
            Send(std::string("goodbye"), from);
        }
    };

    template <class MessageType>
    class Catcher
    {
    public:

        inline Catcher() : mMessage(), mFrom(Theron::Address::Null())
        {
        }

        inline void Catch(const MessageType &message, const Theron::Address from)
        {
            mMessage = message;
            mFrom = from;
        }

        MessageType mMessage;
        Theron::Address mFrom;
    };

    struct Holder
    {
        Theron::ActorRef mRef;
    };

    class Counter : public Theron::Actor
    {
    public:

        inline Counter() : mCount(0)
        {
            RegisterHandler(this, &Counter::Increment);
            RegisterHandler(this, &Counter::GetValue);
        }

    private:

        inline void Increment(const int &message, const Theron::Address /*from*/)
        {
            mCount += message;
        }

        inline void GetValue(const bool &/*message*/, const Theron::Address from)
        {
            Send(mCount, from);
        }

        int mCount;
    };

    class TwoHandlerCounter : public Theron::Actor
    {
    public:

        inline TwoHandlerCounter() : mCount(0)
        {
            RegisterHandler(this, &TwoHandlerCounter::IncrementOne);
            RegisterHandler(this, &TwoHandlerCounter::IncrementTwo);
            RegisterHandler(this, &TwoHandlerCounter::GetValue);
        }

    private:

        inline void IncrementOne(const int &message, const Theron::Address /*from*/)
        {
            mCount += message;
        }

        inline void IncrementTwo(const float &/*message*/, const Theron::Address /*from*/)
        {
            ++mCount;
        }

        inline void GetValue(const bool &/*message*/, const Theron::Address from)
        {
            Send(mCount, from);
        }

        int mCount;
    };

    class MultipleHandlerCounter : public Theron::Actor
    {
    public:

        inline MultipleHandlerCounter() : mCount(0)
        {
            RegisterHandler(this, &MultipleHandlerCounter::IncrementOne);
            RegisterHandler(this, &MultipleHandlerCounter::IncrementTwo);
            RegisterHandler(this, &MultipleHandlerCounter::GetValue);
        }

    private:

        inline void IncrementOne(const int &message, const Theron::Address /*from*/)
        {
            mCount += message;
        }

        inline void IncrementTwo(const int &/*message*/, const Theron::Address /*from*/)
        {
            ++mCount;
        }

        inline void GetValue(const bool &/*message*/, const Theron::Address from)
        {
            Send(mCount, from);
        }

        int mCount;
    };

    template <class CountType>
    class Sequencer : public Theron::Actor
    {
    public:

        static const char *GOOD;
        static const char *BAD;

        inline Sequencer() : mNextValue(0), mStatus(GOOD)
        {
            RegisterHandler(this, &Sequencer::Receive);
            RegisterHandler(this, &Sequencer::GetValue);
        }

    private:

        inline void Receive(const CountType &message, const Theron::Address /*from*/)
        {
            if (message != mNextValue++)
            {
                mStatus = BAD;
            }
        }

        inline void GetValue(const bool &/*message*/, const Theron::Address from)
        {
            Send(mStatus, from);
        }

        CountType mNextValue;
        const char *mStatus;
    };

    class Parameterized : public Theron::Actor
    {
    public:

        struct Parameters
        {
            Theron::Address mAddress;
        };

        inline Parameterized(const Parameters &params) : mStoredAddress(params.mAddress)
        {
            RegisterHandler(this, &Parameterized::Handler);
        }

        inline void Handler(const int &message, const Theron::Address /*from*/)
        {
            // Send a message to the address provided on construction.
            Send(message, mStoredAddress);
        }

    private:

        Theron::Address mStoredAddress;
    };

    class Recursor : public Theron::Actor
    {
    public:

        struct Parameters
        {
            int mCount;
        };

        inline Recursor(const Parameters &params)
        {
            // Recursively create a child actor within the constructor.
            if (params.mCount > 0)
            {
                Parameters childParams;
                childParams.mCount = params.mCount - 1;

                Theron::Framework &framework(GetFramework());
                Theron::ActorRef child(framework.CreateActor<Recursor>(childParams));
            }
        }
    };

    class TailRecursor : public Theron::Actor
    {
    public:

        struct Parameters
        {
            int mCount;
        };

        inline TailRecursor(const Parameters &params) : mCount(params.mCount)
        {
        }

        inline ~TailRecursor()
        {
            // Recursively create a child actor within the destructor.
            if (mCount > 0)
            {
                Parameters childParams;
                childParams.mCount = mCount - 1;

                Theron::Framework &framework(GetFramework());
                Theron::ActorRef child(framework.CreateActor<TailRecursor>(childParams));
            }
        }

    private:

        int mCount;
    };

    class AutoSender : public Theron::Actor
    {
    public:

        typedef Theron::Address Parameters;

        inline AutoSender(const Parameters &address)
        {
            // Send a message in the actor constructor.
            Send(0, address);

            // Send using TailSend to check that works too.
            TailSend(1, address);
        }
    };

    class TailSender : public Theron::Actor
    {
    public:

        typedef Theron::Address Parameters;

        inline TailSender(const Parameters &address) : mStoredAddress(address)
        {
        }

        inline ~TailSender()
        {
            // Send a message in the actor destructor.
            Send(0, mStoredAddress);

            // Send using TailSend to check that works too.
            TailSend(1, mStoredAddress);
        }

        Theron::Address mStoredAddress;
    };

    class AutoDeregistrar : public Theron::Actor
    {
    public:

        inline AutoDeregistrar()
        {
            RegisterHandler(this, &AutoDeregistrar::HandlerOne);
            RegisterHandler(this, &AutoDeregistrar::HandlerTwo);
            DeregisterHandler(this, &AutoDeregistrar::HandlerOne);
        }

        inline void HandlerOne(const int &/*message*/, const Theron::Address from)
        {
            Send(1, from);
        }

        inline void HandlerTwo(const int &/*message*/, const Theron::Address from)
        {
            Send(2, from);
        }
    };

    class TailDeregistrar : public Theron::Actor
    {
    public:

        inline TailDeregistrar()
        {
            RegisterHandler(this, &TailDeregistrar::Handler);
        }

        inline ~TailDeregistrar()
        {
            DeregisterHandler(this, &TailDeregistrar::Handler);
        }

        inline void Handler(const int &/*message*/, const Theron::Address from)
        {
            Send(0, from);
        }
    };

    class MessageQueueCounter : public Theron::Actor
    {
    public:

        inline MessageQueueCounter()
        {
            RegisterHandler(this, &MessageQueueCounter::Handler);
        }

        inline void Handler(const int &/*message*/, const Theron::Address from)
        {
            Send(GetNumQueuedMessages(), from);
        }
    };

    class BlindActor : public Theron::Actor
    {
    public:

        inline BlindActor()
        {
            SetDefaultHandler(this, &BlindActor::BlindDefaultHandler);
        }

    private:

        inline void BlindDefaultHandler(const void *const data, const Theron::uint32_t size, const Theron::Address from)
        {
            // We know the message is a uint32_t.
            const Theron::uint32_t *const p(reinterpret_cast<const Theron::uint32_t *>(data));
            const Theron::uint32_t value(*p);

            Send(value, from);
            Send(size, from);
        }
    };

    template <class MessageType>
    class Accumulator
    {
    public:

        inline Accumulator() : mMessages()
        {
        }

        inline void Catch(const MessageType &message, const Theron::Address /*from*/)
        {
            mMessages.push(message);
        }

        int Size()
        {
            return mMessages.size();
        }

        MessageType Pop()
        {
            THERON_ASSERT(mMessages.empty() == false);
            MessageType message(mMessages.front());
            mMessages.pop();
            return message;
        }

    private:

        std::queue<MessageType> mMessages;
    };

    class HandlerChecker : public Theron::Actor
    {
    public:

        inline HandlerChecker()
        {
            RegisterHandler(this, &HandlerChecker::Check);
        }

    private:

        inline void Check(const int & /*message*/, const Theron::Address from)
        {
            Send(IsHandlerRegistered(this, &HandlerChecker::Check), from);
            Send(IsHandlerRegistered(this, &HandlerChecker::Dummy), from);
            Send(IsHandlerRegistered(this, &HandlerChecker::Unregistered), from);

            RegisterHandler(this, &HandlerChecker::Dummy);

            Send(IsHandlerRegistered(this, &HandlerChecker::Check), from);
            Send(IsHandlerRegistered(this, &HandlerChecker::Dummy), from);
            Send(IsHandlerRegistered(this, &HandlerChecker::Unregistered), from);

            DeregisterHandler(this, &HandlerChecker::Dummy);

            Send(IsHandlerRegistered(this, &HandlerChecker::Check), from);
            Send(IsHandlerRegistered(this, &HandlerChecker::Dummy), from);
            Send(IsHandlerRegistered(this, &HandlerChecker::Unregistered), from);

            DeregisterHandler(this, &HandlerChecker::Check);

            Send(IsHandlerRegistered(this, &HandlerChecker::Check), from);
            Send(IsHandlerRegistered(this, &HandlerChecker::Dummy), from);
            Send(IsHandlerRegistered(this, &HandlerChecker::Unregistered), from);

            RegisterHandler(this, &HandlerChecker::Dummy);
            RegisterHandler(this, &HandlerChecker::Check);
            RegisterHandler(this, &HandlerChecker::Check);

            Send(IsHandlerRegistered(this, &HandlerChecker::Check), from);
            Send(IsHandlerRegistered(this, &HandlerChecker::Dummy), from);
            Send(IsHandlerRegistered(this, &HandlerChecker::Unregistered), from);
        
            DeregisterHandler(this, &HandlerChecker::Check);

            Send(IsHandlerRegistered(this, &HandlerChecker::Check), from);
            Send(IsHandlerRegistered(this, &HandlerChecker::Dummy), from);
            Send(IsHandlerRegistered(this, &HandlerChecker::Unregistered), from);

            DeregisterHandler(this, &HandlerChecker::Check);

            Send(IsHandlerRegistered(this, &HandlerChecker::Check), from);
            Send(IsHandlerRegistered(this, &HandlerChecker::Dummy), from);
            Send(IsHandlerRegistered(this, &HandlerChecker::Unregistered), from);
        }

        inline void Dummy(const int & message, const Theron::Address from)
        {
            // We do this just so that Dummy and Unregistered differ, so the compiler won't merge them!
            Send(message, from);
        }

        inline void Unregistered(const int & /*message*/, const Theron::Address /*from*/)
        {
        }
    };

    class Nestor : public Theron::Actor
    {
    public:

        struct CreateMessage { };
        struct DestroyMessage { };

        inline Nestor()
        {
            RegisterHandler(this, &Nestor::Create);
            RegisterHandler(this, &Nestor::Destroy);
            RegisterHandler(this, &Nestor::Receive);
        }

    private:

        typedef Replier<int> ChildActor;

        inline void Create(const CreateMessage & /*message*/, const Theron::Address /*from*/)
        {
            mChildren.push_back(GetFramework().CreateActor<ChildActor>());
            mChildren.push_back(GetFramework().CreateActor<ChildActor>());
            mChildren.push_back(GetFramework().CreateActor<ChildActor>());

            mReplies.push_back(false);
            mReplies.push_back(false);
            mReplies.push_back(false);

            Send(0, mChildren[0].GetAddress());
            Send(1, mChildren[1].GetAddress());
            Send(2, mChildren[2].GetAddress());
        }

        inline void Destroy(const DestroyMessage & /*message*/, const Theron::Address from)
        {
            mCaller = from;
            if (mReplies[0] && mReplies[1] && mReplies[2])
            {
                mChildren.clear();
                Send(true, mCaller);
            }
        }

        inline void Receive(const int & message, const Theron::Address /*from*/)
        {
            mReplies[message] = true;

            if (mCaller != Theron::Address::Null() && mReplies[0] && mReplies[1] && mReplies[2])
            {
                mChildren.clear();
                Send(true, mCaller);
            }
        }

        std::vector<Theron::ActorRef> mChildren;
        std::vector<bool> mReplies;
        Theron::Address mCaller;
    };

    class FallbackHandler
    {
    public:

        inline void Handle(const Theron::Address from)
        {
            mAddress = from;
        }

        Theron::Address mAddress;
    };

    class BlindFallbackHandler
    {
    public:

        BlindFallbackHandler() : mData(0), mValue(0), mSize(0)
        {
        }

        inline void Handle(const void *const data, const Theron::uint32_t size, const Theron::Address from)
        {
            mData = data;
            mValue = *reinterpret_cast<const Theron::uint32_t *>(data);
            mSize = size;
            mAddress = from;
        }

        const void *mData;
        Theron::uint32_t mValue;
        Theron::uint32_t mSize;
        Theron::Address mAddress;
    };

    class LastGasp : public Theron::Actor
    {
    public:

        typedef Theron::Address Parameters;

        inline explicit LastGasp(const Theron::Address address) : mStoredAddress(address)
        {
        }

        inline ~LastGasp()
        {
            Send(0, mStoredAddress);
        }

    private:

        Theron::Address mStoredAddress;
    };

    typedef std::vector<float> FloatVectorMessage;

    class SomeOtherBaseclass
    {
    public:

        inline SomeOtherBaseclass()
        {
        }

        // The virtual destructor is required because this is a baseclass with virtual functions.
        inline virtual ~SomeOtherBaseclass()
        {
        }

        inline virtual void DoNothing()
        {
        }
    };

    class ActorFirst : public Theron::Actor, public SomeOtherBaseclass
    {
    public:

        inline ActorFirst()
        {
            RegisterHandler(this, &ActorFirst::Handler);
        }

        // The virtual destructor is required because we derived from baseclasses with virtual functions.
        inline virtual ~ActorFirst()
        {
        }

    private:

        inline virtual void DoNothing()
        {
        }

        inline void Handler(const int &message, const Theron::Address from)
        {
            Send(message, from);
        }
    };

    class ActorLast : public SomeOtherBaseclass, public Theron::Actor
    {
    public:

        inline ActorLast()
        {
            RegisterHandler(this, &ActorLast::Handler);
        }

        // The virtual destructor is required because we derived from baseclasses with virtual functions.
        inline virtual ~ActorLast()
        {
        }

    private:

        inline virtual void DoNothing()
        {
        }
        
        inline void Handler(const int &message, const Theron::Address from)
        {
            Send(message, from);
        }
    };

    struct EmptyMessage
    {
    };

    inline LegacyTestSuite()
    {
        TESTFRAMEWORK_REGISTER_TESTSUITE(LegacyTestSuite);

        TESTFRAMEWORK_REGISTER_TEST(NullActorReference);
        TESTFRAMEWORK_REGISTER_TEST(CreateActorInFunction);
        TESTFRAMEWORK_REGISTER_TEST(CreateActorWithParamsInFunction);
        TESTFRAMEWORK_REGISTER_TEST(SendMessageToReceiverInFunction);
        TESTFRAMEWORK_REGISTER_TEST(SendMessageFromNullAddressInFunction);
        TESTFRAMEWORK_REGISTER_TEST(SendMessageToActorFromNullAddressInFunction);
        TESTFRAMEWORK_REGISTER_TEST(SendMessageToActorFromReceiverInFunction);
        TESTFRAMEWORK_REGISTER_TEST(PushMessageToActorFromNullAddressInFunction);
        TESTFRAMEWORK_REGISTER_TEST(PushMessageToActorFromReceiverInFunction);
        TESTFRAMEWORK_REGISTER_TEST(CatchReplyInFunction);
        TESTFRAMEWORK_REGISTER_TEST(StoreActorRefAsMemberInFunction);
        TESTFRAMEWORK_REGISTER_TEST(SendNonPODMessageInFunction);
        TESTFRAMEWORK_REGISTER_TEST(SendConstPointerMessageInFunction);
        TESTFRAMEWORK_REGISTER_TEST(CreateDerivedActor);
        TESTFRAMEWORK_REGISTER_TEST(SendMessageToDerivedActor);
        TESTFRAMEWORK_REGISTER_TEST(IncrementCounter);
        TESTFRAMEWORK_REGISTER_TEST(ActorTemplate);
        TESTFRAMEWORK_REGISTER_TEST(OneHandlerAtATime);
        TESTFRAMEWORK_REGISTER_TEST(MultipleHandlersForMessageType);
        TESTFRAMEWORK_REGISTER_TEST(MessageArrivalOrder);
        TESTFRAMEWORK_REGISTER_TEST(SendAddressAsMessage);
        TESTFRAMEWORK_REGISTER_TEST(SendActorRefAsMessage);
        TESTFRAMEWORK_REGISTER_TEST(SendMessageToDefaultHandlerInFunction);
        TESTFRAMEWORK_REGISTER_TEST(RegisterHandlerFromHandler);
        TESTFRAMEWORK_REGISTER_TEST(CreateActorInConstructor);
        TESTFRAMEWORK_REGISTER_TEST(SendMessageInConstructor);
        TESTFRAMEWORK_REGISTER_TEST(DeregisterHandlerInConstructor);
        TESTFRAMEWORK_REGISTER_TEST(CreateActorInDestructor);
        TESTFRAMEWORK_REGISTER_TEST(SendMessageInDestructor);
        TESTFRAMEWORK_REGISTER_TEST(DeregisterHandlerInDestructor);
        TESTFRAMEWORK_REGISTER_TEST(CreateActorInHandler);
        TESTFRAMEWORK_REGISTER_TEST(GetNumQueuedMessagesInHandler);
        TESTFRAMEWORK_REGISTER_TEST(GetNumQueuedMessagesInFunction);
        TESTFRAMEWORK_REGISTER_TEST(UseBlindDefaultHandler);
        TESTFRAMEWORK_REGISTER_TEST(IsHandlerRegisteredInHandler);
        TESTFRAMEWORK_REGISTER_TEST(SetFallbackHandler);
        TESTFRAMEWORK_REGISTER_TEST(HandleUnhandledMessageSentInFunction);
        TESTFRAMEWORK_REGISTER_TEST(SendRegisteredMessage);
        TESTFRAMEWORK_REGISTER_TEST(DeriveFromActorFirst);
        TESTFRAMEWORK_REGISTER_TEST(DeriveFromActorLast);
        TESTFRAMEWORK_REGISTER_TEST(SendEmptyMessage);
    }

    inline static void NullActorReference()
    {
        Theron::ActorRef nullReference;
        Check(nullReference == Theron::ActorRef::Null(), "Default-constructed reference isn't null");
        Check((nullReference != Theron::ActorRef::Null()) == false, "Default-constructed reference isn't null");
    }

    inline static void CreateActorInFunction()
    {
        Theron::Framework framework;
        Theron::ActorRef actor(framework.CreateActor<Trivial>());
    }

    inline static void CreateActorWithParamsInFunction()
    {
        Theron::Framework framework;
        Theron::Receiver receiver;

        Parameterized::Parameters params;
        params.mAddress = receiver.GetAddress();
        Theron::ActorRef actor(framework.CreateActor<Parameterized>(params));

        framework.Send(0, Theron::Address::Null(), actor.GetAddress());

        receiver.Wait();
    }

    inline static void SendMessageToReceiverInFunction()
    {
        Theron::Framework framework;
        Theron::Receiver receiver;
        framework.Send(0.0f, receiver.GetAddress(), receiver.GetAddress());
    }

    inline static void SendMessageFromNullAddressInFunction()
    {
        Theron::Framework framework;
        Theron::Receiver receiver;

        framework.Send(0, Theron::Address::Null(), receiver.GetAddress());
        receiver.Wait();
    }

    inline static void SendMessageToActorFromNullAddressInFunction()
    {
        Theron::Framework framework;
        Theron::Receiver receiver;
        Theron::ActorRef signaler(framework.CreateActor<Signaler>());

        framework.Send(receiver.GetAddress(), Theron::Address::Null(), signaler.GetAddress());
        receiver.Wait();
    }

    inline static void SendMessageToActorFromReceiverInFunction()
    {
        Theron::Framework framework;
        Theron::Receiver receiver;
        Theron::ActorRef signaler(framework.CreateActor<Signaler>());

        framework.Send(receiver.GetAddress(), receiver.GetAddress(), signaler.GetAddress());
        receiver.Wait();
    }

    inline static void PushMessageToActorFromNullAddressInFunction()
    {
        Theron::Framework framework;
        Theron::Receiver receiver;
        Theron::ActorRef signaler(framework.CreateActor<Signaler>());

        signaler.Push(receiver.GetAddress(), Theron::Address::Null());
        receiver.Wait();
    }

    inline static void PushMessageToActorFromReceiverInFunction()
    {
        Theron::Framework framework;
        Theron::Receiver receiver;
        Theron::ActorRef signaler(framework.CreateActor<Signaler>());

        signaler.Push(receiver.GetAddress(), receiver.GetAddress());
        receiver.Wait();
    }

    inline static void ReceiveReplyInFunction()
    {
        typedef Replier<float> FloatReplier;

        Theron::Framework framework;
        Theron::Receiver receiver;
        Theron::ActorRef actor(framework.CreateActor<FloatReplier>());

        framework.Send(5.0f, receiver.GetAddress(), actor.GetAddress());

        receiver.Wait();
    }

    inline static void CatchReplyInFunction()
    {
        typedef Replier<float> FloatReplier;
        typedef Catcher<float> FloatCatcher;

        Theron::Framework framework;
        Theron::ActorRef actor(framework.CreateActor<FloatReplier>());

        Theron::Receiver receiver;
        FloatCatcher catcher;
        receiver.RegisterHandler(&catcher, &FloatCatcher::Catch);

        framework.Send(5.0f, receiver.GetAddress(), actor.GetAddress());

        receiver.Wait();

        Check(catcher.mMessage == 5.0f, "Caught message value wrong");
        Check(catcher.mFrom == actor.GetAddress(), "Caught from address wrong");
    }

    inline static void StoreActorRefAsMemberInFunction()
    {
        typedef Replier<float> FloatReplier;

        Theron::Framework framework;
        Theron::Receiver receiver;

        Holder object;
        object.mRef = framework.CreateActor<FloatReplier>();

        framework.Send(5.0f, receiver.GetAddress(), object.mRef.GetAddress());

        receiver.Wait();
    }

    inline static void SendNonPODMessageInFunction()
    {
        typedef std::vector<int> VectorMessage;
        typedef Replier<VectorMessage> VectorReplier;
        typedef Catcher<VectorMessage> VectorCatcher;

        Theron::Framework framework;
        Theron::ActorRef actor(framework.CreateActor<VectorReplier>());

        Theron::Receiver receiver;
        VectorCatcher catcher;
        receiver.RegisterHandler(&catcher, &VectorCatcher::Catch);

        VectorMessage vectorMessage;
        vectorMessage.push_back(0);
        vectorMessage.push_back(1);
        vectorMessage.push_back(2);
        framework.Send(vectorMessage, receiver.GetAddress(), actor.GetAddress());

        receiver.Wait();

        Check(catcher.mMessage == vectorMessage, "Reply message is wrong");
    }

    inline static void SendPointerMessageInFunction()
    {
        typedef float * PointerMessage;
        typedef Replier<PointerMessage> PointerReplier;
        typedef Catcher<PointerMessage> PointerCatcher;

        Theron::Framework framework;
        Theron::ActorRef actor(framework.CreateActor<PointerReplier>());

        Theron::Receiver receiver;
        PointerCatcher catcher;
        receiver.RegisterHandler(&catcher, &PointerCatcher::Catch);

        float a(0.0f);
        PointerMessage pointerMessage(&a);
        framework.Send(pointerMessage, receiver.GetAddress(), actor.GetAddress());

        receiver.Wait();

        Check(catcher.mMessage == &a, "Reply message is wrong");
    }

    inline static void SendConstPointerMessageInFunction()
    {
        typedef const float * PointerMessage;
        typedef Replier<PointerMessage> PointerReplier;
        typedef Catcher<PointerMessage> PointerCatcher;

        Theron::Framework framework;
        Theron::ActorRef actor(framework.CreateActor<PointerReplier>());

        Theron::Receiver receiver;
        PointerCatcher catcher;
        receiver.RegisterHandler(&catcher, &PointerCatcher::Catch);

        float a(0.0f);
        PointerMessage pointerMessage(&a);
        framework.Send(pointerMessage, receiver.GetAddress(), actor.GetAddress());

        receiver.Wait();

        Check(catcher.mMessage == &a, "Reply message is wrong");
    }

    inline static void CreateDerivedActor()
    {
        Theron::Framework framework;
        Theron::ActorRef actor(framework.CreateActor<StringReplier>());
    }

    inline static void SendMessageToDerivedActor()
    {
        typedef const char * StringMessage;
        typedef Catcher<StringMessage> StringCatcher;

        Theron::Framework framework;
        Theron::ActorRef actor(framework.CreateActor<StringReplier>());
    
        Theron::Receiver receiver;
        StringCatcher catcher;
        receiver.RegisterHandler(&catcher, &StringCatcher::Catch);
    
        const char *testString = "hello";
        StringMessage stringMessage(testString);
        framework.Send(stringMessage, receiver.GetAddress(), actor.GetAddress());

        receiver.Wait();

        Check(catcher.mMessage == stringMessage, "Reply message is wrong");
    }

    inline static void IncrementCounter()
    {
        typedef Catcher<int> CountCatcher;

        Theron::Framework framework;
        Theron::ActorRef actor(framework.CreateActor<Counter>());
    
        Theron::Receiver receiver;
        CountCatcher catcher;
        receiver.RegisterHandler(&catcher, &CountCatcher::Catch);

        framework.Send(1, receiver.GetAddress(), actor.GetAddress());
        framework.Send(2, receiver.GetAddress(), actor.GetAddress());
        framework.Send(3, receiver.GetAddress(), actor.GetAddress());
        framework.Send(4, receiver.GetAddress(), actor.GetAddress());
        framework.Send(5, receiver.GetAddress(), actor.GetAddress());
        framework.Send(6, receiver.GetAddress(), actor.GetAddress());

        framework.Send(true, receiver.GetAddress(), actor.GetAddress());

        receiver.Wait();

        Check(catcher.mMessage == 21, "Count is wrong");
    }

    inline static void ActorTemplate()
    {
        typedef Replier<int> IntReplier;

        Theron::Framework framework;
        Theron::ActorRef actor(framework.CreateActor<IntReplier>());

        Theron::Receiver receiver;
        framework.Send(10, receiver.GetAddress(), actor.GetAddress());

        receiver.Wait();
    }

    inline static void OneHandlerAtATime()
    {
        typedef Catcher<int> CountCatcher;

        Theron::Framework framework;
        Theron::ActorRef actor(framework.CreateActor<TwoHandlerCounter>());

        Theron::Receiver receiver;
        CountCatcher catcher;
        receiver.RegisterHandler(&catcher, &CountCatcher::Catch);

        framework.Send(2, receiver.GetAddress(), actor.GetAddress());
        framework.Send(0.0f, receiver.GetAddress(), actor.GetAddress());
        framework.Send(2, receiver.GetAddress(), actor.GetAddress());
        framework.Send(0.0f, receiver.GetAddress(), actor.GetAddress());
        framework.Send(2, receiver.GetAddress(), actor.GetAddress());
        framework.Send(0.0f, receiver.GetAddress(), actor.GetAddress());

        // Get the counter value.
        framework.Send(true, receiver.GetAddress(), actor.GetAddress());

        receiver.Wait();

        Check(catcher.mMessage == 9, "Count is wrong");
    }

    inline static void MultipleHandlersForMessageType()
    {
        typedef Catcher<int> CountCatcher;

        Theron::Framework framework;
        Theron::ActorRef actor(framework.CreateActor<MultipleHandlerCounter>());

        Theron::Receiver receiver;
        CountCatcher catcher;
        receiver.RegisterHandler(&catcher, &CountCatcher::Catch);

        framework.Send(2, receiver.GetAddress(), actor.GetAddress());
        framework.Send(2, receiver.GetAddress(), actor.GetAddress());
        framework.Send(2, receiver.GetAddress(), actor.GetAddress());

        // Get the counter value.
        framework.Send(true, receiver.GetAddress(), actor.GetAddress());

        receiver.Wait();

        Check(catcher.mMessage == 9, "Count is wrong");
    }

    inline static void MessageArrivalOrder()
    {
        typedef Catcher<const char *> StringCatcher;
        typedef Sequencer<int> IntSequencer;

        Theron::Framework framework;
        Theron::ActorRef actor(framework.CreateActor<IntSequencer>());

        Theron::Receiver receiver;
        StringCatcher catcher;
        receiver.RegisterHandler(&catcher, &StringCatcher::Catch);

        framework.Send(0, receiver.GetAddress(), actor.GetAddress());
        framework.Send(1, receiver.GetAddress(), actor.GetAddress());
        framework.Send(2, receiver.GetAddress(), actor.GetAddress());
        framework.Send(3, receiver.GetAddress(), actor.GetAddress());
        framework.Send(4, receiver.GetAddress(), actor.GetAddress());
        framework.Send(5, receiver.GetAddress(), actor.GetAddress());
        framework.Send(6, receiver.GetAddress(), actor.GetAddress());
        framework.Send(7, receiver.GetAddress(), actor.GetAddress());

        // Get the validity value.
        framework.Send(true, receiver.GetAddress(), actor.GetAddress());

        receiver.Wait();
        Check(catcher.mMessage == IntSequencer::GOOD, "Sequencer status is wrong");

        framework.Send(9, receiver.GetAddress(), actor.GetAddress());

        // Get the validity value.
        framework.Send(true, receiver.GetAddress(), actor.GetAddress());

        receiver.Wait();
        Check(catcher.mMessage == IntSequencer::BAD, "Sequencer status is wrong");
    }

    inline static void SendAddressAsMessage()
    {
        typedef Catcher<Theron::Address> AddressCatcher;

        Theron::Framework framework;
        Theron::ActorRef actorA(framework.CreateActor<Signaler>());
        Theron::ActorRef actorB(framework.CreateActor<Signaler>());

        Theron::Receiver receiver;
        AddressCatcher catcher;
        receiver.RegisterHandler(&catcher, &AddressCatcher::Catch);

        // Send A a message telling it to signal B.
        // A sends the receiver address to B as the signal,
        // causing B to send A's address to the receiver in turn.
        framework.Send(actorB.GetAddress(), receiver.GetAddress(), actorA.GetAddress());

        receiver.Wait();

        Check(catcher.mMessage == actorA.GetAddress(), "Wrong address");
    }

    inline static void SendActorRefAsMessage()
    {
        typedef Catcher<Theron::Address> AddressCatcher;

        Theron::Framework framework;
        Theron::ActorRef actorA(framework.CreateActor<Poker>());
        Theron::ActorRef actorB(framework.CreateActor<Signaler>());

        Theron::Receiver receiver;
        AddressCatcher catcher;
        receiver.RegisterHandler(&catcher, &AddressCatcher::Catch);

        // Send A a reference to B, telling it to poke B.
        // A sends the receiver address to B as the poke,
        // causing B to send A's address to the receiver in turn.
        framework.Send(actorB, receiver.GetAddress(), actorA.GetAddress());

        receiver.Wait();

        Check(catcher.mMessage == actorA.GetAddress(), "Wrong address");
    }

    inline static void SendMessageToDefaultHandlerInFunction()
    {
        typedef DefaultReplier<float> FloatReplier;
        typedef Catcher<std::string> StringCatcher;

        Theron::Framework framework;
        Theron::ActorRef actor(framework.CreateActor<FloatReplier>());

        StringCatcher catcher;
        Theron::Receiver receiver;
        receiver.RegisterHandler(&catcher, &StringCatcher::Catch);

        // Send an int to the replier, which expects floats but has a default handler.
        framework.Send(52, receiver.GetAddress(), actor.GetAddress());

        receiver.Wait();

        Check(catcher.mMessage == "hello", "Default handler not executed");
    }

    inline static void RegisterHandlerFromHandler()
    {
        typedef Catcher<std::string> StringCatcher;

        Theron::Framework framework;
        Theron::ActorRef actor(framework.CreateActor<Switcher>());

        StringCatcher catcher;
        Theron::Receiver receiver;
        receiver.RegisterHandler(&catcher, &StringCatcher::Catch);

        framework.Send(std::string("hello"), receiver.GetAddress(), actor.GetAddress());
        receiver.Wait();
        Check(catcher.mMessage == "hello", "Handler not executed");

        framework.Send(std::string("hello"), receiver.GetAddress(), actor.GetAddress());
        receiver.Wait();
        Check(catcher.mMessage == "goodbye", "Handler not executed");

        framework.Send(std::string("hello"), receiver.GetAddress(), actor.GetAddress());
        receiver.Wait();
        Check(catcher.mMessage == "hello", "Handler not executed");

        framework.Send(std::string("hello"), receiver.GetAddress(), actor.GetAddress());
        receiver.Wait();
        Check(catcher.mMessage == "goodbye", "Handler not executed");
    }

    inline static void CreateActorInConstructor()
    {
        Theron::Framework framework;

        Recursor::Parameters params;
        params.mCount = 10;
        Theron::ActorRef actor(framework.CreateActor<Recursor>(params));
    }

    inline static void SendMessageInConstructor()
    {
        Theron::Framework framework;
        Theron::Receiver receiver;

        // Pass the address of the receiver to the actor constructor.
        AutoSender::Parameters params(receiver.GetAddress());
        Theron::ActorRef actor(framework.CreateActor<AutoSender>(params));

        // Wait for the messages sent by the actor on construction.
        receiver.Wait();
        receiver.Wait();
    }

    inline static void DeregisterHandlerInConstructor()
    {
        typedef Catcher<int> IntCatcher;

        Theron::Framework framework;

        Theron::Receiver receiver;
        IntCatcher catcher;
        receiver.RegisterHandler(&catcher, &IntCatcher::Catch);

        Theron::ActorRef actor(framework.CreateActor<AutoDeregistrar>());

        // Send the actor a message and check that the first handler doesn't send us a reply.
        framework.Send(0, receiver.GetAddress(), actor.GetAddress());

        receiver.Wait();
        Check(catcher.mMessage == 2, "Received wrong message");
        Check(receiver.Count() == 0, "Received too many messages");
    }

    inline static void CreateActorInDestructor()
    {
        Theron::Framework framework;

        TailRecursor::Parameters params;
        params.mCount = 10;
        Theron::ActorRef actor(framework.CreateActor<TailRecursor>(params));
    }

    inline static void SendMessageInDestructor()
    {
        Theron::Framework framework;
        Theron::Receiver receiver;

        {
            // Pass the address of the receiver ton the actor constructor.
            TailSender::Parameters params(receiver.GetAddress());
            Theron::ActorRef actor(framework.CreateActor<TailSender>(params));
        }

        // Wait for the messages sent by the actor on construction.
        receiver.Wait();
        receiver.Wait();
    }

    inline static void DeregisterHandlerInDestructor()
    {
        // We check that it's safe to deregister a handler in an actor destructor,
        // but since it can't handle messages after destruction, there's little effect.
        Theron::Framework framework;
        Theron::ActorRef actor(framework.CreateActor<TailDeregistrar>());
    }

    inline static void CreateActorInHandler()
    {
        Theron::Framework framework;
        Theron::Receiver receiver;
        Theron::ActorRef actor(framework.CreateActor<Nestor>());

        framework.Send(Nestor::CreateMessage(), receiver.GetAddress(), actor.GetAddress());
        framework.Send(Nestor::DestroyMessage(), receiver.GetAddress(), actor.GetAddress());

        receiver.Wait();
    }

    inline static void GetNumQueuedMessagesInHandler()
    {
        typedef Catcher<Theron::uint32_t> CountCatcher;

        Theron::Framework framework;
        Theron::Receiver receiver;

        CountCatcher catcher;
        receiver.RegisterHandler(&catcher, &CountCatcher::Catch);
        
        Theron::ActorRef actor(framework.CreateActor<MessageQueueCounter>());

        // Send the actor two messages.
        framework.Send(0, receiver.GetAddress(), actor.GetAddress());
        framework.Send(0, receiver.GetAddress(), actor.GetAddress());

        // Wait for and check both replies.
        // Race condition decides whether the second message has already arrived.
        // In Theron 4, the count includes the message currently being handled.
        receiver.Wait();
        Check(catcher.mMessage == 2 || catcher.mMessage == 1, "Bad count");

        receiver.Wait();
        Check(catcher.mMessage == 1, "Bad count");
    }

    inline static void GetNumQueuedMessagesInFunction()
    {
        typedef const char * StringMessage;
        typedef Catcher<Theron::uint32_t> CountCatcher;

        Theron::Framework framework;
        Theron::Receiver receiver;

        Theron::ActorRef actor(framework.CreateActor<StringReplier>());

        // Send the actor two messages.
        StringMessage stringMessage("hello");
        framework.Send(stringMessage, receiver.GetAddress(), actor.GetAddress());
        framework.Send(stringMessage, receiver.GetAddress(), actor.GetAddress());

        // Race conditions decide how many messages are in the queue when we ask.
        // In Theron 4 the count includes the message currently being processed.
        Theron::uint32_t numMessages(actor.GetNumQueuedMessages());
        Check(numMessages < 3, "GetNumQueuedMessages failed, expected less than 3 messages");

        receiver.Wait();

        // There's no guarantee that the message handler will finish before the Wait() call returns.
        numMessages = actor.GetNumQueuedMessages();
        Check(numMessages < 3, "GetNumQueuedMessages failed, expected less than 3 messages");

        receiver.Wait();

        numMessages = actor.GetNumQueuedMessages();
        Check(numMessages < 2, "GetNumQueuedMessages failed, expected less than 2 messages");
    }

    inline static void UseBlindDefaultHandler()
    {
        typedef Accumulator<Theron::uint32_t> UIntAccumulator;

        Theron::Framework framework;
        Theron::Receiver receiver;

        UIntAccumulator accumulator;
        receiver.RegisterHandler(&accumulator, &UIntAccumulator::Catch);
        
        Theron::ActorRef actor(framework.CreateActor<BlindActor>());

        // Send the actor a uint32_t message, which is the type it secretly expects.
        framework.Send(Theron::uint32_t(75), receiver.GetAddress(), actor.GetAddress());

        // The actor sends back the value of the message data and the size.
        receiver.Wait();
        receiver.Wait();

        Check(accumulator.Pop() == 75, "Bad blind data");
        Check(accumulator.Pop() == 4, "Bad blind data size");
    }

    inline static void IsHandlerRegisteredInHandler()
    {
        typedef Accumulator<bool> BoolAccumulator;

        Theron::Framework framework;
        Theron::Receiver receiver;

        BoolAccumulator accumulator;
        receiver.RegisterHandler(&accumulator, &BoolAccumulator::Catch);
        
        Theron::ActorRef actor(framework.CreateActor<HandlerChecker>());
        framework.Send(int(0), receiver.GetAddress(), actor.GetAddress());

        Theron::uint32_t count(21);
        while (count)
        {
            count -= receiver.Wait(count);
        }

        Check(accumulator.Pop() == true, "Bad registration check result 0");
        Check(accumulator.Pop() == false, "Bad registration check result 1");
        Check(accumulator.Pop() == false, "Bad registration check result 2");

        //RegisterHandler(this, &HandlerChecker::Dummy);
        Check(accumulator.Pop() == true, "Bad registration check result 3");
        Check(accumulator.Pop() == true, "Bad registration check result 4");
        Check(accumulator.Pop() == false, "Bad registration check result 5");

        // DeregisterHandler(this, &HandlerChecker::Dummy);
        Check(accumulator.Pop() == true, "Bad registration check result 6");
        Check(accumulator.Pop() == false, "Bad registration check result 7");
        Check(accumulator.Pop() == false, "Bad registration check result 8");

        // DeregisterHandler(this, &HandlerChecker::Check);
        Check(accumulator.Pop() == false, "Bad registration check result 9");
        Check(accumulator.Pop() == false, "Bad registration check result 10");
        Check(accumulator.Pop() == false, "Bad registration check result 11");

        // RegisterHandler(this, &HandlerChecker::Dummy);
        // RegisterHandler(this, &HandlerChecker::Check);
        // RegisterHandler(this, &HandlerChecker::Check);
        Check(accumulator.Pop() == true, "Bad registration check result 12");
        Check(accumulator.Pop() == true, "Bad registration check result 13");
        Check(accumulator.Pop() == false, "Bad registration check result 14");

        // DeregisterHandler(this, &HandlerChecker::Check);
        Check(accumulator.Pop() == true, "Bad registration check result 15");
        Check(accumulator.Pop() == true, "Bad registration check result 16");
        Check(accumulator.Pop() == false, "Bad registration check result 17");

        // DeregisterHandler(this, &HandlerChecker::Check);
        Check(accumulator.Pop() == false, "Bad registration check result 18");
        Check(accumulator.Pop() == true, "Bad registration check result 19");
        Check(accumulator.Pop() == false, "Bad registration check result 20");
    }

    inline static void SetFallbackHandler()
    {
        Theron::Framework framework;
        FallbackHandler fallbackHandler;

        Check(framework.SetFallbackHandler(&fallbackHandler, &FallbackHandler::Handle), "Register failed");
    }

    inline static void HandleUnhandledMessageSentInFunction()
    {
        typedef Replier<Theron::uint32_t> UIntReplier;

        Theron::Framework framework;
        Theron::Receiver receiver;

        FallbackHandler fallbackHandler;
        framework.SetFallbackHandler(&fallbackHandler, &FallbackHandler::Handle);

        // Create a replier that handles only ints, then send it a float.
        Theron::ActorRef replier(framework.CreateActor<UIntReplier>());
        framework.Send(5.0f, receiver.GetAddress(), replier.GetAddress());

        // Send the replier an int and wait for the reply so we know both messages
        // have been processed.
        framework.Send(Theron::uint32_t(5), receiver.GetAddress(), replier.GetAddress());
        receiver.Wait();

        // Check that the unhandled message was handled by the registered fallback handler.
        Check(fallbackHandler.mAddress == receiver.GetAddress(), "Fallback handler failed");
    }

    inline static void SendRegisteredMessage()
    {
        typedef Replier<FloatVectorMessage> IntVectorReplier;
        typedef Catcher<FloatVectorMessage> IntVectorCatcher;

        Theron::Framework framework;

        Theron::Receiver receiver;
        IntVectorCatcher catcher;
        receiver.RegisterHandler(&catcher, &IntVectorCatcher::Catch);

        Theron::ActorRef replier(framework.CreateActor<IntVectorReplier>());

        FloatVectorMessage message;
        message.push_back(0.0f);
        message.push_back(1.0f);
        message.push_back(2.0f);

        framework.Send(message, receiver.GetAddress(), replier.GetAddress());
        receiver.Wait();

        Check(catcher.mMessage.size() == 3, "Bad reply message");
        Check(catcher.mMessage[0] == 0, "Bad reply message");
        Check(catcher.mMessage[1] == 1, "Bad reply message");
        Check(catcher.mMessage[2] == 2, "Bad reply message");
    }

    inline static void DeriveFromActorFirst()
    {
        typedef Catcher<int> IntCatcher;

        Theron::Framework framework;

        Theron::Receiver receiver;
        IntCatcher catcher;
        receiver.RegisterHandler(&catcher, &IntCatcher::Catch);

        Theron::ActorRef actor(framework.CreateActor<ActorFirst>());

        framework.Send(5, receiver.GetAddress(), actor.GetAddress());
        receiver.Wait();

        Check(catcher.mMessage == 5, "Bad reply message");
    }

    inline static void DeriveFromActorLast()
    {
        typedef Catcher<int> IntCatcher;

        Theron::Framework framework;

        Theron::Receiver receiver;
        IntCatcher catcher;
        receiver.RegisterHandler(&catcher, &IntCatcher::Catch);

        Theron::ActorRef actor(framework.CreateActor<ActorLast>());

        framework.Send(5, receiver.GetAddress(), actor.GetAddress());
        receiver.Wait();

        Check(catcher.mMessage == 5, "Bad reply message");
    }

    inline static void SendEmptyMessage()
    {
        typedef Replier<EmptyMessage> EmptyReplier;
        typedef Catcher<EmptyMessage> EmptyCatcher;

        Theron::Framework framework;

        Theron::Receiver receiver;
        EmptyCatcher catcher;
        receiver.RegisterHandler(&catcher, &EmptyCatcher::Catch);

        Theron::ActorRef replier(framework.CreateActor<EmptyReplier>());

        framework.Send(EmptyMessage(), receiver.GetAddress(), replier.GetAddress());
        receiver.Wait();

        Check(&catcher.mMessage != 0, "No reply message");
    }
};


template <class CountType>
const char *LegacyTestSuite::Sequencer<CountType>::GOOD = "good";

template <class CountType>
const char *LegacyTestSuite::Sequencer<CountType>::BAD = "good";


} // namespace Tests


THERON_REGISTER_MESSAGE(Tests::LegacyTestSuite::FloatVectorMessage);


#endif // THERON_TESTS_TESTSUITES_LEGACYTESTSUITE_H
