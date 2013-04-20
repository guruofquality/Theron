// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_ALLOCATORMANAGER_H
#define THERON_ALLOCATORMANAGER_H


/**
\file AllocatorManager.h
Manager for allocators used within Theron.
*/


#include <Theron/Assert.h>
#include <Theron/DefaultAllocator.h>
#include <Theron/Defines.h>
#include <Theron/IAllocator.h>


namespace Theron
{


/**
\brief Singleton class that manages allocators for use by Theron.

This class is a singleton, and its single instance can be accessed using the
static \ref Instance method on the class.

Non-static \ref SetAllocator and \ref GetAllocator methods on the singleton instance
allow the allocator used by Theron to be set and retrieved. Setting the allocator
replaces the \ref DefaultAllocator, which is used if no custom allocator is explicitly
set. The \ref GetAllocator method returns a pointer to the currently set allocator,
which is either the allocator set previously using \ref SetAllocator or the \ref
DefaultAllocator, if none has been set.

\code
class MyAllocator : public Theron::IAllocator
{
public:

    MyAllocator();
    virtual ~MyAllocator();

    virtual void *Allocate(const SizeType size);
    virtual void *AllocateAligned(const SizeType size, const SizeType alignment);
    virtual void Free(void *const memory);
};

MyAllocator allocator;
Theron::AllocatorManager::Instance().SetAllocator(&allocator);
\endcode

\note The \ref SetAllocator method can be called at most once, at application start.
If the \ref DefaultAllocator is replaced with a custom allocator then it must be
replaced at application start, before any Theron objects (\ref Framework "frameworks",
\ref Actor "actors" or \ref Receiver "receivers") are constructed. \ref GetAllocator
can be called any number of times, both before and after a call to \ref SetAllocator.
*/
class AllocatorManager
{
public:

    /**
    \brief Returns a reference to the AllocatorManager singleton instance.
    */
    THERON_FORCEINLINE static AllocatorManager &Instance()
    {
        return smInstance;
    }

    /**
    \brief Sets the allocator used for internal allocations, replacing the default allocator.

    Calling this method allows applications to provide custom allocators and hence to control
    how and where memory is allocated by Theron. This is useful, for example, in embedded systems
    where memory is scarce or of different available types. The allocator provided to this method
    must implement IAllocator and can be a wrapper around another existing allocator implementation.
    The provided allocator is used for all internal heap allocations, including the allocation of
    instantiated actors. If this method is not called by user code then a default DefaultAllocator
    is used, which is a simple wrapper around global new and delete.

    \code
    MyAllocator allocator;
    Theron::AllocatorManager::Instance().SetAllocator(&allocator);
    \endcode
    
    \note This method should be called once at most, and before any other Theron activity.

    \see GetAllocator
    */
    inline void SetAllocator(IAllocator *const allocator)
    {
        // This method should only be called once, at start of day.
        THERON_ASSERT_MSG(mAllocator == &mDefaultAllocator, "SetAllocator can only be called once!");
        THERON_ASSERT_MSG(mDefaultAllocator.GetBytesAllocated() == 0, "SetAllocator can only be called before Framework construction");
        THERON_ASSERT(allocator != 0);

        mAllocator = allocator;
    }

    /**
    \brief Gets a pointer to the general allocator currently in use by Theron.

    \code
    Theron::IAllocator *const allocator = Theron::AllocatorManager::Instance().GetAllocator();
    Theron::DefaultAllocator *const defaultAllocator = dynamic_cast<Theron::DefaultAllocator *>(allocator);

    if (defaultAllocator)
    {
        printf("Default allocator has %d bytes currently allocated\n", defaultAllocator->GetBytesAllocated());
    }
    \endcode

    \see SetAllocator
    */
    THERON_FORCEINLINE IAllocator *GetAllocator() const
    {
        return mAllocator;
    }

private:

    /**
    Default constructor. Private, since the AllocatorManager is a singleton class.
    */
    inline AllocatorManager() :
      mDefaultAllocator(),
      mAllocator(&mDefaultAllocator)
    {
    }

    AllocatorManager(const AllocatorManager &other);
    AllocatorManager &operator=(const AllocatorManager &other);

    static AllocatorManager smInstance;         ///< The single, static instance.

    DefaultAllocator mDefaultAllocator;         ///< Default allocator used if no user allocator is set.
    IAllocator *mAllocator;                     ///< Pointer to a general allocator for use in internal allocations.
};


} // namespace Theron


#endif // THERON_ALLOCATORMANAGER_H
