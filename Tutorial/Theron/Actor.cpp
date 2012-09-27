// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#include <Theron/Actor.h>
#include <Theron/Assert.h>
#include <Theron/Framework.h>

#include <Theron/Detail/Legacy/ActorRegistry.h>


namespace Theron
{


Actor::Actor() :
  mAddress(),
  mFramework(0),
  mMessageHandlers(),
  mDefaultHandlers(),
  mProcessorContext(0),
  mRefCount(0),
  mMemory(0)
{
    // This is the legacy way of constructing actors, from versions prior to 4.0.
    // Lookup the register entry for this actor in the ActorRegistry.
    // The entry should have been created by the owning Framework in CreateActor.
    // The entry passes us a pointer to the owning Framework.
    // We can't pass it through the default constructor so need this nasty kludge.
    Detail::ActorRegistry::Entry *const entry(Detail::ActorRegistry::Lookup(this));

    // If this assert fires that probably means the user is trying to use the default
    // constructor of the Theron::Actor baseclass, which isn't allowed. Instead,
    // use the explicit constructor and pass it a Framework object in the initializer
    // list of the derived actor constructor:
    //
    // class MyActor : public Theron::Actor
    // {
    // public:
    //     MyActor(Theron::Framework &framework) : Theron::Actor(framework)
    //     {
    //     }
    // };

    THERON_ASSERT_MSG(entry, "Actor baseclass not initialized. Use Theron::Actor::Actor(TheronFramework &framework).");

    // Check that the entry is valid.
    THERON_ASSERT(entry->mActor == this);
    THERON_ASSERT(entry->mFramework != 0);
    THERON_ASSERT(entry->mMemory != 0);

    // Load the framework pointer from the entry.
    mFramework = entry->mFramework;
    mMemory = entry->mMemory;

    // Claim an available directory index and mailbox for this actor.
    mFramework->RegisterActor(this);
}


Actor::Actor(Framework &framework, const char *const name) :
  mAddress(),
  mFramework(&framework),
  mMessageHandlers(),
  mDefaultHandlers(),
  mProcessorContext(0),
  mRefCount(0),
  mMemory(0)
{
    // Check that the actor isn't being constructed by Framework::CreateActor.
    THERON_ASSERT_MSG(Detail::ActorRegistry::Lookup(this) == 0, "Use default Theron::Actor::Actor() baseclass constructor with CreateActor().");

    // Claim an available directory index and mailbox for this actor.
    mFramework->RegisterActor(this, name);
}


Actor::~Actor()
{
    mFramework->DeregisterActor(this);
}


void Actor::Fallback(
    Detail::FallbackHandlerCollection *const fallbackHandlers,
    const Detail::IMessage *const message)
{
    // If default handlers are registered with this actor, execute those.
    if (mDefaultHandlers.Handle(this, message))
    {
        return;
    }

    // Let the framework's fallback handlers handle the message.
    fallbackHandlers->Handle(message);
}


} // namespace Theron
