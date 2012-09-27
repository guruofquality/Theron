// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_REGISTER_H
#define THERON_REGISTER_H


#include <Theron/Detail/Messages/MessageTraits.h>


/**
\file Register.h
Optional type registration for message types.
*/


/**
\def THERON_REGISTER_MESSAGE

\brief Message type registration macro.

Registers message types for use within Theron.

Registration of message types is optional in Theron. Registering the message
types used in an application causes Theron to use hand-rolled Runtime Type
Information (RTTI) explicitly stored with each message type, instead of the
built-in C++ RTTI.

Doing so allows the built-in C++ RTTI to be turned off, resulting in a memory
storage saving in all types, not just message types. This saving can be important
in embedded and real-time applications such as games, where for example a Vector
class may need to be stored as just three floats, with no additional runtime
type information added silently by the compiler.

Another useful benefit is the avoidance of the C++ dynamic_cast operator, which
is often slow in many compilers. When THERON_REGISTER_MESSAGE is used, Theron
rolls its own type ID system, and its own type ID checking is significantly
faster than dynamic_cast. (Looking at the compiled code for dynamic_cast in
Visual Studio, for example, it's hard to imagine how it can be so long).

An important limitation of the message type registration macro is that it
can only be used from within the global namespace. Furthermore the full
name of the message type must be given, including all namespace scoping.
(Unfortunately this means that it isn't generally possible to register
messages immediately after their declaration, as we'd often prefer).

\code
namespace MyNamespace
{

class MyMessage
{
};

}

THERON_REGISTER_MESSAGE(MyNamespace::MyMessage);
\endcode

If message types are registered, then \em all message types
must be registered, throughout the whole application, otherwise runtime
errors will occur. Therefore it is simpler and less error prone to not
register any message types at all. Only bother with message registration
if you specifically need to turn the built-in C++ RTTI system off.
If you decide to register your message types, you can turn on message
registration checking, which helps to detect unregistered message types.
Define \ref THERON_ENABLE_MESSAGE_REGISTRATION_CHECKS as 1 globally in your
build, ideally via build settings.

\note With message registration, the final type of each message must
be unique. If two different message types (ie, with different scoped names)
are actually identical, then the attempt to register the second type will
fail with a confusing message about a template already having been defined.
For example, this implies that two structs with identical contents are the
same message type, even if they have different names.
*/


#ifndef THERON_REGISTER_MESSAGE

#define THERON_REGISTER_MESSAGE(MessageType)                                \
namespace Theron                                                            \
{                                                                           \
namespace Detail                                                            \
{                                                                           \
template <>                                                                 \
struct MessageTraits<MessageType>                                           \
{                                                                           \
    static const bool HAS_TYPE_NAME = true;                                 \
    static const char *const TYPE_NAME;                                     \
};                                                                          \
                                                                            \
const char *const MessageTraits<MessageType>::TYPE_NAME = #MessageType;     \
}                                                                           \
}

#endif // THERON_REGISTER_MESSAGE


#endif // THERON_REGISTER_H
