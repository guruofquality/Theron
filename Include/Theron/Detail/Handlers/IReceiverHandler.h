// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_DETAIL_HANDLERS_IRECEIVERHANDLER_H
#define THERON_DETAIL_HANDLERS_IRECEIVERHANDLER_H


#include <Theron/BasicTypes.h>
#include <Theron/Defines.h>

#include <Theron/Detail/Messages/IMessage.h>


namespace Theron
{
namespace Detail
{


/// Baseclass that allows message handlers of various types to be stored in lists.
class IReceiverHandler
{
public:

    /// Default constructor.
    THERON_FORCEINLINE IReceiverHandler() : mNext(0)
    {
    }

    /// Virtual destructor.
    inline virtual ~IReceiverHandler()
    {
    }

    /// Sets the pointer to the next message handler in a list of handlers.
    inline void SetNext(IReceiverHandler *const next);

    /// Gets the pointer to the next message handler in a list of handlers.
    inline IReceiverHandler *GetNext() const;

    /// Returns the unique name of the message type handled by this handler.
    virtual const char *GetMessageTypeName() const = 0;

    /// Handles the given message, if it's of the type accepted by the handler.
    /// \return True, if the handler handled the message.
    virtual bool Handle(const IMessage *const message) const = 0;

private:

    IReceiverHandler(const IReceiverHandler &other);
    IReceiverHandler &operator=(const IReceiverHandler &other);

    IReceiverHandler *mNext;    ///< Pointer to the next handler in a list of handlers.
};


THERON_FORCEINLINE void IReceiverHandler::SetNext(IReceiverHandler *const next)
{
    mNext = next;
}


THERON_FORCEINLINE IReceiverHandler *IReceiverHandler::GetNext() const
{
    return mNext;
}


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_HANDLERS_IRECEIVERHANDLER_H

