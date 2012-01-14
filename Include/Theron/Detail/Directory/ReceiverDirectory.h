// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_DETAIL_DIRECTORY_RECEIVERDIRECTORY_H
#define THERON_DETAIL_DIRECTORY_RECEIVERDIRECTORY_H


#include <Theron/Detail/BasicTypes.h>
#include <Theron/Detail/Debug/Assert.h>
#include <Theron/Detail/PagedPool/PagedPool.h>
#include <Theron/Detail/Threading/Mutex.h>

#include <Theron/Address.h>
#include <Theron/Defines.h>


namespace Theron
{


class Receiver;


namespace Detail
{


/// A directory mapping unique addresses to receivers.
class ReceiverDirectory
{
public:

    /// Gets a reference to the single directory object.
    inline static ReceiverDirectory &Instance();

    /// Default constructor.
    inline ReceiverDirectory();

    /// Returns the number of receivers currently registered.
    inline uint32_t Count() const;

    /// Registers a receiver and returns its unique address.
    Address RegisterReceiver(Receiver *const receiver);

    /// Deregisters the receiver at the given address.
    /// \note The address can be the address of a currently registered receiver.
    bool DeregisterReceiver(const Address &address);

    /// Gets a pointer to the receiver at the given address.
    /// \note The address can be the address of a currently registered receiver.
    Receiver *GetReceiver(const Address &address) const;

private:

    typedef PagedPool<Receiver *, THERON_MAX_RECEIVERS> ReceiverPool;

    ReceiverDirectory(const ReceiverDirectory &other);
    ReceiverDirectory &operator=(const ReceiverDirectory &other);

    static ReceiverDirectory smInstance;    ///< Single, static instance of the class.

    ReceiverPool mReceiverPool;             ///< Pool of pointers to user-allocated receivers.
};


THERON_FORCEINLINE ReceiverDirectory &ReceiverDirectory::Instance()
{
    return smInstance;
}


THERON_FORCEINLINE ReceiverDirectory::ReceiverDirectory() : mReceiverPool()
{
}


THERON_FORCEINLINE uint32_t ReceiverDirectory::Count() const
{
    return mReceiverPool.Count();
}


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_DIRECTORY_RECEIVERDIRECTORY_H

