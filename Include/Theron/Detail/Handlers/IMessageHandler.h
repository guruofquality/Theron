// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_DETAIL_HANDLERS_IMESSAGEHANDLER_H
#define THERON_DETAIL_HANDLERS_IMESSAGEHANDLER_H


#include <Theron/BasicTypes.h>
#include <Theron/Defines.h>

#include <Theron/Detail/Messages/IMessage.h>


namespace Theron
{


class Actor;


namespace Detail
{


/// Baseclass that allows message handlers of various types to be stored in lists.
class IMessageHandler
{
public:

    /// Default constructor.
    THERON_FORCEINLINE IMessageHandler() : mNext(0), mMarked(false)
    {
    }

    /// Virtual destructor.
    inline virtual ~IMessageHandler()
    {
    }

    /// Sets the pointer to the next message handler in a list of handlers.
    inline void SetNext(IMessageHandler *const next);

    /// Gets the pointer to the next message handler in a list of handlers.
    inline IMessageHandler *GetNext() const;

    /// Marks the handler (eg. for deletion).    
    inline void Mark();

    /// Returns true if the handler is marked (eg. for deletion).    
    inline bool IsMarked() const;

    /// Returns the unique name of the message type handled by this handler.
    virtual const char *GetMessageTypeName() const = 0;

    /// Handles the given message, if it's of the type accepted by the handler.
    /// \return True, if the handler handled the message.
    virtual bool Handle(Actor *const actor, const IMessage *const message) const = 0;

private:

    IMessageHandler(const IMessageHandler &other);
    IMessageHandler &operator=(const IMessageHandler &other);

    IMessageHandler *mNext; ///< Pointer to the next handler in a list of handlers.
    bool mMarked;           ///< Flag used to mark the handler for deletion.
};


THERON_FORCEINLINE void IMessageHandler::SetNext(IMessageHandler *const next)
{
    mNext = next;
}


THERON_FORCEINLINE IMessageHandler *IMessageHandler::GetNext() const
{
    return mNext;
}


THERON_FORCEINLINE void IMessageHandler::Mark()
{
    mMarked = true;
}


THERON_FORCEINLINE bool IMessageHandler::IsMarked() const
{
    return mMarked;
}


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_HANDLERS_IMESSAGEHANDLER_H

