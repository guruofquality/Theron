// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_ADDRESS_H
#define THERON_ADDRESS_H


/**
\file Address.h
Address object by which messages are addressed.
*/


#include <Theron/BasicTypes.h>
#include <Theron/Defines.h>


namespace Theron
{


namespace Detail
{
class MessageSender;
}


class Framework;
class Receiver;


/**
\brief The unique address of an entity that can send or receive messages.

Addresses are the unique 'names' of entities that can receive messages in Theron.
Knowing an entity's address is enough to be able to send it a message, and no other
special access or permissions are required.

The types of entities that have addresses, and therefore can receive messages, in
Theron are \ref Actor "actors" and \ref Receiver "receivers". Both of these types
of entities are assigned automatically-generated unique addresses on construction.

Addresses can be copied and assigned, allowing the addresses of actors and receivers
to be \ref Actor::Send "sent in messages" to other actors. The only way to generate
a valid address is by querying and copying the address of an actor or receiver,
ensuring that addresses remain unique.

Default-constructed addresses are \em null and are equal to the unique null address
returned by the static \ref Address::Null method, which is guaranteed not to be
equal to the address of any addressable entity.

Because knowing the address of an actor means being able to send it a message,
addresses constitute the security and encapsulation mechanism within \ref Actor "actor"
subsystems. If an actor creates a collection of 'child' actors that it owns and manages,
then no code outside that actor can access the child actors or send them messages,
unless the actor publicly exposes either the actors or their addresses.
*/
class Address
{
public:

    friend class Detail::MessageSender;
    friend class Framework;
    friend class Receiver;

    /**
    \brief Static method that returns the unique 'null' address.

    The null address is guaranteed not to be equal to the address of any actual entity.

    \code
    class Actor : public Theron::Actor
    {
        explicit Actor(Theron::Framework &framework) : Theron::Actor(framework)
        {
        }
    };

    Theron::Framework framework;
    Actor actor(framework);

    assert(actor.GetAddress() != Address::Null(), "Created actor has null address!");
    \endcode

    \return The unique null address.
    */
    THERON_FORCEINLINE static Address Null()
    {
        return Address();
    }

    /**
    \brief Default constructor.

    Constructs a null address, equal to the address returned by Address::Null().
    */
    THERON_FORCEINLINE Address() : mValue()
    {
    }

    /**
    \brief Copy constructor.

    Addresses can be assigned. The new address is equal to the original and identifies the same entity.

    \code
    class Actor : public Theron::Actor
    {
        explicit Actor(Theron::Framework &framework) : Theron::Actor(framework)
        {
        }
    };

    Theron::Framework framework;
    Actor actor(framework);

    // Copy-construct the actor's address.
    Theron::Address address(actor.GetAddress());
    \endcode

    \param other The existing address to be copied.
    */
    THERON_FORCEINLINE Address(const Address &other) : mValue(other.mValue)
    {
    }

    /**
    \brief Assignment operator.

    Addresses can be assigned. The new address is equal to the original and identifies the same entity.

    \code
    class Actor : public Theron::Actor
    {
        explicit Actor(Theron::Framework &framework) : Theron::Actor(framework)
        {
        }
    };

    Theron::Framework framework;
    Actor actor(framework);
    
    // Assign the actor's address into a default-constructed address.
    Theron::Address address;
    address = actor.GetAddress();
    \endcode

    \param other The existing address whose value is to be copied.
    */
    THERON_FORCEINLINE Address &operator=(const Address &other)
    {
        mValue = other.mValue;
        return *this;
    }

    /**
    \brief Gets an integer index identifying the host containing the addressed entity.

    \code
    class Actor : public Theron::Actor
    {
        explicit Actor(Theron::Framework &framework) : Theron::Actor(framework)
        {
        }
    };

    Theron::Framework framework;
    Actor actor(framework);

    printf("Actor is in host %lu\n", actor.GetAddress().GetHost());
    \endcode

    \note At present, Theron doesn't support multiple hosts, and the results of this
    method are undefined.
    */
    THERON_FORCEINLINE uint32_t GetHost() const
    {
        return mValue.mComponents.mGlobal.mBits.mHost;
    }

    /**
    \brief Gets an integer index identifying the process containing the addressed entity.

    \code
    class Actor : public Theron::Actor
    {
        explicit Actor(Theron::Framework &framework) : Theron::Actor(framework)
        {
        }
    };

    Theron::Framework framework;
    Actor actor(framework);

    printf("Actor is in process %lu\n", actor.GetAddress().GetProcess());
    \endcode

    \note At present, Theron doesn't support multiple processes, and the results of this
    method are undefined.
    */
    THERON_FORCEINLINE uint32_t GetProcess() const
    {
        return mValue.mComponents.mGlobal.mBits.mProcess;
    }

    /**
    \brief Gets an integer index identifying the framework containing the addressed entity.

    \code
    class Actor : public Theron::Actor
    {
        explicit Actor(Theron::Framework &framework) : Theron::Actor(framework)
        {
        }
    };

    Theron::Framework framework;
    Actor actor(framework);

    printf("Actor is in framework %lu\n", actor.GetAddress().GetFramework());
    \endcode

    \note A value of zero indicates that the entity is a Receiver, which is global to the
    current process and not associated with any specific framework.
    */
    THERON_FORCEINLINE uint32_t GetFramework() const
    {
        return mValue.mComponents.mLocal.mBits.mFramework;
    }

    /**
    \brief Gets the value of the address as an unsigned 32-bit integer.

    \note The returned integer value of an address represents its index within the
    framework within which it was created, and isn't unique across multiple frameworks
    within the same process, across multiple processes within the same host, or across
    multiple hosts. In order to uniquely characterize actors across all frameworks within
    a process, either use \ref AsUInt64 or combine the value returned by AsInteger with
    the value returned by \ref GetFramework, which uniquely identifies the framework
    hosting the actor within the current process.

    \code
    class Actor : public Theron::Actor
    {
        explicit Actor(Theron::Framework &framework) : Theron::Actor(framework)
        {
        }
    };

    Theron::Framework framework;
    Actor actor(framework);

    printf("Actor has address %lu\n", actor.GetAddress().AsInteger());
    \endcode

    \see AsUInt64
    */
    THERON_FORCEINLINE uint32_t AsInteger() const
    {
        return mValue.mComponents.mLocal.mBits.mIndex;
    }

    /**
    \brief Gets the unique value of the address as an unsigned 64-bit integer.

    \note The 64-bit value of an address is guaranteed to be unique even across multiple hosts
    or multiple processes within the same host. Currently Theron doesn't support multiple
    processors or multiple hosts, so this value is trivially unique for every actor.

    \code
    class Actor : public Theron::Actor
    {
        explicit Actor(Theron::Framework &framework) : Theron::Actor(framework)
        {
        }
    };

    Theron::Framework framework;
    Actor actor(framework);

    printf("Actor has address %llu\n", actor.GetAddress().AsUInt64());
    \endcode

    \see AsInteger
    */
    THERON_FORCEINLINE uint64_t AsUInt64() const
    {
        return mValue.mUInt64;
    }

    /**
    \brief Equality operator.

    Returns true if two addresses are the same, otherwise false.

    \param other The address to be compared.
    \return True if the given address is the same as this one.
    */
    THERON_FORCEINLINE bool operator==(const Address &other) const
    {
        return (mValue == other.mValue);
    }

    /**
    \brief Inequality operator.

    Returns true if two addresses are different, otherwise false.

    \param other The address to be compared.
    \return True if the given address is different from this one.
    */
    THERON_FORCEINLINE bool operator!=(const Address &other) const
    {
        return (mValue != other.mValue);
    }

    /**
    \brief Less-than operator.

    This allows addresses to be sorted, for example in containers.

    \code
    class Actor : public Theron::Actor
    {
        explicit Actor(Theron::Framework &framework) : Theron::Actor(framework)
        {
        }
    };

    Theron::Framework framework;
    Actor actorOne(framework);
    Actor actorTwo(framework);

    std::set<Theron::Address> addresses;
    addresses.insert(actorOne.GetAddress());
    addresses.insert(actorTwo.GetAddress());
    \endcode
    */
    THERON_FORCEINLINE bool operator<(const Address &other) const
    {
        return (mValue < other.mValue);
    }

private:

    /**
    Internal explicit constructor, used by friend classes.
    */
    THERON_FORCEINLINE explicit Address(const uint32_t framework, const uint32_t index) : mValue(framework, index)
    {
    }

    struct GlobalComponents
    {
        uint32_t mHost : 16;       ///< Integer index identifying the host within a distributed network.
        uint32_t mProcess : 16;    ///< Integer index identifying the process within the host.
    };

    struct LocalComponents
    {
        uint32_t mFramework : 12;  ///< Integer index identifying the framework within the process (zero indicates a receiver).
        uint32_t mIndex : 20;      ///< Integer index with the framework.
    };

    struct Components
    {
        union
        {
            GlobalComponents mBits;
            uint32_t mUInt32;

        } mGlobal;

        union
        {
            LocalComponents mBits;
            uint32_t mUInt32;

        } mLocal;
    };

    union Value
    {
        THERON_FORCEINLINE Value() : mUInt64(0)
        {
        }

        THERON_FORCEINLINE Value(const uint32_t framework, const uint32_t index) : mUInt64(0)
        {
            mComponents.mLocal.mBits.mFramework = framework;
            mComponents.mLocal.mBits.mIndex = index;
        }

        THERON_FORCEINLINE Value(const Value &other) : mUInt64(other.mUInt64)
        {
        }

        THERON_FORCEINLINE Value &operator=(const Value &other)
        {
             mUInt64 = other.mUInt64;
             return *this;
        }

        THERON_FORCEINLINE bool operator==(const Value &other) const
        {
            return (mUInt64 == other.mUInt64);
        }

        THERON_FORCEINLINE bool operator!=(const Value &other) const
        {
            return (mUInt64 != other.mUInt64);
        }

        THERON_FORCEINLINE bool operator<(const Value &other) const
        {
            return (mUInt64 < other.mUInt64);
        }

        uint64_t mUInt64;           ///< Unique unsigned 64-bit value.
        Components mComponents;     ///< Individual components of the address.
    };

    Value mValue;                   ///< 64-bit address value, accessible as bitfield components.
};


} // namespace Theron


#endif // THERON_ADDRESS_H
