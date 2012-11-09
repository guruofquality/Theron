// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_DETAIL_NETWORK_STRING_H
#define THERON_DETAIL_NETWORK_STRING_H


#include <new>

#include <string.h>
#include <stdlib.h>

#include <Theron/AllocatorManager.h>
#include <Theron/Assert.h>
#include <Theron/BasicTypes.h>
#include <Theron/Defines.h>
#include <Theron/IAllocator.h>

#include <Theron/Detail/Threading/Atomic.h>
#include <Theron/Detail/Threading/Utils.h>


#ifdef _MSC_VER
#pragma warning(push)
#pragma warning (disable:4996)  // function or variable may be unsafe.
#pragma warning (disable:4324)  // structure was padded due to __declspec(align())
#endif //_MSC_VER


namespace Theron
{
namespace Detail
{


/**
A copyable string type that is a lightweight wrapper around a reference-counted body.
*/
class String
{
public:

    /**
    Default constructor.
    Constructs a null string with no value.
    */
    THERON_FORCEINLINE String() : mBody(0)
    {
    }

    /**
    Explicit constructor.
    */
    THERON_FORCEINLINE explicit String(const char *const str) : mBody(0)
    {
        // Create and reference a new body.
        if (str)
        {
            IAllocator *const allocator(AllocatorManager::Instance().GetAllocator());
            void *const bodyMemory(allocator->AllocateAligned(sizeof(Body), THERON_CACHELINE_ALIGNMENT));
            mBody = new (bodyMemory) Body(str);
        }

        Reference();
    }

    /**
    Copy constructor.
    */
    THERON_FORCEINLINE String(const String &other) : mBody(other.mBody)
    {
        Reference();
    }

    /**
    Assignment operator.
    */
    THERON_FORCEINLINE String &operator=(const String &other)
    {
        Dereference();
        mBody = other.mBody;
        Reference();

        return *this;
    }

    /**
    Destructor.
    */
    THERON_FORCEINLINE ~String()
    {
        Dereference();
    }

    /**
    Returns true if the string has no value.
    */
    THERON_FORCEINLINE bool IsNull() const
    {
        return (mBody == 0);
    }

    /**
    Gets the value of the string.
    \note Returns a null pointer if the string is null.
    */
    THERON_FORCEINLINE const char *GetValue() const
    {
        if (mBody)
        {
            return mBody->GetValue();
        }

        return 0;
    }

    /**
    Equality operator.
    */
    THERON_FORCEINLINE bool operator==(const String &other) const
    {
        if (IsNull() && other.IsNull())
        {
            return true;
        }

        if (!IsNull() && !other.IsNull())
        {
            return (*mBody == *other.mBody);
        }

        return false;
    }

    /**
    Inequality operator.
    */
    THERON_FORCEINLINE bool operator!=(const String &other) const
    {
        return !operator==(other);
    }

    /**
    Less-than operator, mainly for sorted containers.
    */
    THERON_FORCEINLINE bool operator<(const String &other) const
    {
        if (IsNull() && other.IsNull())
        {
            return false;
        }

        if (IsNull())
        {
            return true;
        }

        if (other.IsNull())
        {
            return false;
        }

        return (*mBody < *other.mBody);
    }

private:

    /**
    Reference-counted string body.
    */
    class Body
    {
    public:

        THERON_FORCEINLINE Body(const char *const str) :
          mRefCount(0),
          mValue(0)
        {
            if (strlen(str) + 1 < DATA_SIZE)
            {
                strcpy(mData, str);
                mValue = mData;
            }
            else
            {
                mValue = CopyString(str);
            }            
        }

        THERON_FORCEINLINE ~Body()
        {
            if (mValue != mData)
            {
                DestroyString(mValue);
            }
        }

        THERON_FORCEINLINE void Reference() const
        {
            mRefCount.Increment();
        }

        THERON_FORCEINLINE bool Dereference() const
        {
            uint32_t currentValue(mRefCount.Load());
            uint32_t newValue(currentValue - 1);
            uint32_t backoff(0);

            THERON_ASSERT(currentValue > 0);

            // Repeatedly try to atomically decrement the reference count.
            // We do this by hand so we can atomically find out the new value.
            while (!mRefCount.CompareExchangeAcquire(currentValue, newValue))
            {
                currentValue = mRefCount.Load();
                newValue = currentValue - 1;
                Detail::Utils::Backoff(backoff);
            }

            // Return true if the new reference count is zero.
            return (newValue == 0);
        }

        THERON_FORCEINLINE const char *GetValue() const
        {
            return mValue;
        }

        THERON_FORCEINLINE bool operator==(const Body &other) const
        {
            THERON_ASSERT(mValue);
            THERON_ASSERT(other.mValue);
            return (strcmp(mValue, other.mValue) == 0);
        }

        THERON_FORCEINLINE bool operator!=(const Body &other) const
        {
            return !operator==(other);
        }

        THERON_FORCEINLINE bool operator<(const Body &other) const
        {
            THERON_ASSERT(mValue);
            THERON_ASSERT(other.mValue);
            return (strcmp(mValue, other.mValue) < 0);
        }

    private:

        Body(const Body &other);
        Body &operator=(const Body &other);

        static const uint32_t DATA_SIZE = 48;

        //
        // The total size of this member data should fit in a cache line
        // We've assumed here that a cache line is 64 bytes and a pointer is 8 bytes.
        //

        mutable Detail::Atomic::UInt32 mRefCount;   ///< Counts the number of String objects referencing this Body.
        char *mValue;                               ///< Pointer to a buffer containing the value of the string.
        char mData[DATA_SIZE];                      ///< Local data buffer used to hold small string values.
    };
    
    THERON_FORCEINLINE void Reference()
    {
        if (mBody)
        {
            mBody->Reference();
        }
    }

    THERON_FORCEINLINE void Dereference()
    {
        if (mBody)
        {
            // Destroy the body if dereferencing it makes it unreferenced.
            if (mBody->Dereference())
            {
                IAllocator *const allocator(AllocatorManager::Instance().GetAllocator());
                mBody->~Body();
                allocator->Free(mBody, sizeof(Body));
            }
        }
    }

    THERON_FORCEINLINE static char *CopyString(const char *const str)
    {
        THERON_ASSERT(str);
        
        IAllocator *const allocator(AllocatorManager::Instance().GetAllocator());
        const uint32_t roundedStringSize(RoundedStringSize(str));
        void *const stringMemory(allocator->Allocate(roundedStringSize));
        char *const newStr(reinterpret_cast<char *>(stringMemory));

        if (newStr)
        {
            newStr[0] = '\0';
            strcpy(newStr, str);
        }

        return newStr;
    }

    THERON_FORCEINLINE static void DestroyString(char *const str)
    {
        IAllocator *const allocator(AllocatorManager::Instance().GetAllocator());
        THERON_ASSERT(str);
        allocator->Free(str);
    }

    THERON_FORCEINLINE static uint32_t RoundedStringSize(const char *const str)
    {
        THERON_ASSERT(str);

        const uint32_t stringLength(strlen(str));
        const uint32_t stringSize(stringLength + 1);
        const uint32_t roundedStringSize(THERON_ROUNDUP(stringSize, 4));

        return roundedStringSize;
    }

    Body *mBody;
};


} // namespace Detail
} // namespace Theron


#ifdef _MSC_VER
#pragma warning(pop)
#endif //_MSC_VER


#endif // THERON_DETAIL_NETWORK_STRING_H
