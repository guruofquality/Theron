// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_DETAIL_HANDLERS_IDEFAULTHANDLER_H
#define THERON_DETAIL_HANDLERS_IDEFAULTHANDLER_H


#include <Theron/Detail/Messages/IMessage.h>


namespace Theron
{


class Actor;


namespace Detail
{


/// Interface that allows a default handler on an unknown actor to be referenced.
class IDefaultHandler
{
public:

    /// Default constructor.
    inline IDefaultHandler()
    {
    }

    /// Virtual destructor.
    inline virtual ~IDefaultHandler()
    {
    }

    /// Handles the given message.
    virtual void Handle(Actor *const actor, const IMessage *const message) const = 0;
};


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_HANDLERS_IDEFAULTHANDLER_H

