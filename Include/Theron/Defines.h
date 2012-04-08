// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_DEFINES_H
#define THERON_DEFINES_H


/**
\file Defines.h
\brief Global user-overridable defines.

This file defines, in one place, all the defines which can be defined in order
to override default options within Theron. Some of them enable or disable debugging
functionality, others abstract away platform-specific detail such as code inlining
and variable alignment, and still others control the operation of Theron such as
the limits on the number of threads per \ref Framework and the total number of \ref Actor
"Actors" and \ref Receiver "Receivers" that can be created in an application.

\note The intention is that most users will be able to leave these defines at their default
values, hardly knowing they're there. But if you do need to override one of them then
the best way to do it is to define the define globally in your build (for example as
a compiler option), rather than by editing this file. The definitions of the defines
in this file are all conditional on the define having not already been defined, so can
be easily overridden just by defining the defines yourself globally in your build.
*/


#ifndef THERON_DEBUG
#ifdef _MSC_VER
#if defined(_DEBUG)
#define THERON_DEBUG 1
#else
#define THERON_DEBUG 0
#endif
#elif defined(__GNUC__)
#if defined(NDEBUG)
#define THERON_DEBUG 0
#else
#define THERON_DEBUG 1
#endif
#else
#if defined(_DEBUG)
#define THERON_DEBUG 1
#else
/**
\brief Controls generation of code for asserts, allocation checking, etc.

This is a global define that decides default values for several other
defines that individually control aspects of code generation related to debugging.

The value of \ref THERON_DEBUG can be overridden by defining it globally in the build
(in the makefile using -D, or in the project preprocessor settings in Visual Studio).

If not defined, then \ref THERON_DEBUG defaults to 1 in GCC builds when NDEBUG is not
defined, and to 1 in other builds (including Visual C++) when _DEBUG is defined.
Otherwise it defaults to 0, disabling debug-related code generation.

If \ref THERON_DEBUG is defined as 1, then \ref THERON_ENABLE_ASSERTS defaults to 1,
\ref THERON_FORCEINLINE defaults to 'inline', and \ref
THERON_ENABLE_DEFAULTALLOCATOR_CHECKS defaults to 1. Otherwise those defines
default to 0, forced inlining, and 0, respectively.
*/
#define THERON_DEBUG 0
#endif
#endif
#endif // THERON_DEBUG


#ifndef THERON_ENABLE_ASSERTS
#if THERON_DEBUG
#define THERON_ENABLE_ASSERTS 1
#else
/**
\brief Enables generation of code for asserts within Theron.

Defaults to 0, disabling asserts, when \ref THERON_DEBUG is 0.
Defaults to 1, enabling asserts, when \ref THERON_DEBUG is 1.

The value of \ref THERON_ENABLE_ASSERTS can be overridden by defining it globally
in the build (in the makefile using -D, or in the project preprocessor settings
in Visual Studio).
*/
#define THERON_ENABLE_ASSERTS 0
#endif // THERON_DEBUG
#endif // THERON_ENABLE_ASSERTS


#ifndef THERON_USE_BOOST_THREADS
/**
\brief Controls whether Boost threads, locks, etc are used.

If \ref THERON_USE_BOOST_THREADS is defined as 1 then Boost threads are used, introducing
a dependency on boost::thread. If both THERON_USE_BOOST_THREADS and \ref THERON_USE_STD_THREADS
are defined as 0, Windows threads are used, introducing a dependency on the windows.h header.

Defaults to 0 (Windows threads or std threads).

The value of \ref THERON_USE_BOOST_THREADS can be overridden by defining it globally
in the build (in the makefile using -D, or in the project preprocessor settings
in Visual Studio).

\note The makefile build defines \ref THERON_USE_BOOST_THREADS as 1 by default, overriding
the default value defined in the code. This is arranged this way on the assumption that
users who are using the makefile build are likely to want to avoid a dependency on Windows.
The value defined by the makefile can be controlled by specifying <code>threads=boost</code>
or <code>threads=windows</code> on the make command line.
*/
#define THERON_USE_BOOST_THREADS 0
#endif // THERON_USE_BOOST_THREADS


#ifndef THERON_USE_STD_THREADS
/**
\brief Controls whether std threads, locks, etc are used.

If \ref THERON_USE_BOOST_THREADS is defined as 0 and \ref THERON_USE_STD_THREADS is defined
as 1 then std threads are used, introducing a dependency on std::thread. If both
\ref THERON_USE_BOOST_THREADS and \ref THERON_USE_STD_THREADS are defined as 0, Windows threads
are used, introducing a dependency on the windows.h header.

Defaults to 0 (Windows threads or Boost threads).

The value of \ref THERON_USE_STD_THREADS can be overridden by defining it globally
in the build (in the makefile using -D, or in the project preprocessor settings
in Visual Studio).

\note Support for std::thread is only available in C++11, so requires a C++11 compiler
(for example GCC 4.6 or later).
*/
#define THERON_USE_STD_THREADS 0
#endif // THERON_USE_STD_THREADS


#if THERON_USE_BOOST_THREADS
#if !defined(BOOST_THREAD_BUILD_DLL) && !defined(BOOST_THREAD_BUILD_LIB)
/**
\brief Controls whether boost::thread is built as a static library or dynamic/shared library.

Unless specified otherwise, Theron defaults to building boost::thread as a library.
Somewhat bizarrely Boost defaults to dynamically linked under gcc, in Boost 1.47,
without this define.
*/
#define BOOST_THREAD_BUILD_LIB 1
#endif // !defined(BOOST_THREAD_BUILD_DLL) && !defined(BOOST_THREAD_BUILD_LIB)
#endif // THERON_USE_BOOST_THREADS


#ifndef THERON_FORCEINLINE
#if THERON_DEBUG
#define THERON_FORCEINLINE inline
#else // THERON_DEBUG
#ifdef _MSC_VER
#define THERON_FORCEINLINE __forceinline
#elif defined(__GNUC__)
#define THERON_FORCEINLINE inline __attribute__((always_inline))
#else
/**
\brief Controls force-inlining of core functions within Theron.

Many functions within Theron are force-inlined via compiler keywords, to avoid
function call overhead, resulting in a significant speedup over optionally inlined code.
The use of forced inlining does however lead to some code bloat. Therefore it may
be desirable to turn it off in some builds. Moreover, it makes debugging difficult when
enabled, so it is useful to be able to disable it, and desirable for it to be disabled,
by default, in debug builds.

\ref THERON_FORCEINLINE defines the keyword used for inlined function definitions.
It defaults to __forceinline for Visual C++, and inline __attribute__((always_inline))
for gcc, when \ref THERON_DEBUG is 0. Otherwise it defaults to the inline keyword, obligatory
on functions defined in headers.

The definition of \ref THERON_FORCEINLINE can be overridden by defining it globally
in the build (in the makefile using -D, or in the project preprocessor settings
in Visual Studio).
*/
#define THERON_FORCEINLINE inline
#endif
#endif // THERON_DEBUG
#endif // THERON_FORCEINLINE


#ifndef THERON_ENABLE_DEFAULTALLOCATOR_CHECKS
// Support THERON_ENABLE_SIMPLEALLOCATOR_CHECKS as a legacy synonym.
#if defined(THERON_ENABLE_SIMPLEALLOCATOR_CHECKS)
#define THERON_ENABLE_DEFAULTALLOCATOR_CHECKS THERON_ENABLE_SIMPLEALLOCATOR_CHECKS
#else
#if THERON_DEBUG
#define THERON_ENABLE_DEFAULTALLOCATOR_CHECKS 1
#else
/**
\brief Enables debug checking of allocations in the \ref Theron::DefaultAllocator "DefaultAllocator".

This define controls the use of debugging checks in the default allocator used within
Theron. By default, this define is set to 0 when \ref THERON_DEBUG is 0, and 1 when \ref THERON_DEBUG is 1,
so that debug checks are enabled only in debug builds. However the define can be defined explicitly,
either to suppress the checking in debug builds or enforce it even in release builds, as required.

The value of \ref THERON_ENABLE_DEFAULTALLOCATOR_CHECKS can be overridden by defining it
globally in the build (in the makefile using -D, or in the project preprocessor
settings in Visual Studio).
*/
#define THERON_ENABLE_DEFAULTALLOCATOR_CHECKS 0
#endif // THERON_DEBUG
#endif // defined(THERON_ENABLE_SIMPLEALLOCATOR_CHECKS)
#endif // THERON_ENABLE_DEFAULTALLOCATOR_CHECKS


#ifndef THERON_ENABLE_MESSAGE_REGISTRATION_CHECKS
/**
\brief Enables run-time reporting of unregistered message types, which is off by default.

Message types can be registered using the \ref THERON_REGISTER_MESSAGE macro, which notifies
Theron of the unique name of a message type. Registering the message types sent within an
application causes Theron's internal type ID system to be used to identity messages when they
arrive at an actor, instead of the built-in C++ typeid system.

This has two useful effects: Firstly, it avoids calling the slow C++ dynamic_cast
operator, and instead performs a simple check on the unique name associated with each message type
(a fast pointer comparison). Secondly, it allows the C++ compiler's built-in Run-Time Type
Information (RTTI) system to be disabled, which avoids storing compiler-generated type IDs in
classes.

However this efficiency comes with a catch: once we start registering message types, we
have to remember to consistently always register *every* message type used by the application.
If any of the message types are omitted, then errors may ensue (usually caught by asserts,
in debug builds).

To make this easier, define \ref THERON_ENABLE_MESSAGE_REGISTRATION_CHECKS as 1 in your local
build (ideally globally with a compiler option). This enables run-time error reports that
helpfully detect message types that haven't been registered.

Of course, you should only enable this checking if you are actually intending to register your
message types! If you don't care about the overheads of RTTI and dynamic_cast, then it's perfectly
fine (and a lot simpler) to just not register *any* message types at all -- and leave this define
disabled -- and then everything will work as normal.

\note Note that unregistered message types are reported by asserts, so are only active if assert
code is also enabled via \ref THERON_ENABLE_ASSERTS.

\see <a href="http://www.theron-library.com/index.php?t=page&p=RegisteringMessages">Registering messages</a>
*/
#define THERON_ENABLE_MESSAGE_REGISTRATION_CHECKS 0
#endif // THERON_ENABLE_MESSAGE_REGISTRATION_CHECKS


#ifndef THERON_ENABLE_UNHANDLED_MESSAGE_CHECKS
/**
\brief Enables reporting of undelivered and unhandled messages, which is enabled by default.

This define controls reporting of messages that are either not delivered (because no entity
exists at the address to which the message is sent, in which case \ref Theron::Actor::Send "Actor::Send"
and \ref Theron::Framework::Send "Framework::Send" return false), or not handled by the actor to
which they were delivered (because the actor has no message handlers registered for the message type
and no default message handler).

Such messages are passed to a \ref Theron::Framework::SetFallbackHandler "fallback handler"
registered with the \ref Theron::Framework "Framework" concerned (that within which the message was sent,
in the case of undelivered messages, and that containing the unresponsive actor, in the case of
unhandled messages). The default fallback handler registered with each framework, which is
used unless replaced explicitly by a custom user-specified fallback handler, reports such
messages to stderr using printf, and asserts, if asserts are enabled with \ref THERON_ENABLE_ASSERTS.

This define controls the activation of the default fallback handler. If defined to 1, the
default fallback handler reports failures. If defined to 0, it does nothing, and unhandled
messages are silently unreported. Defining \ref THERON_ENABLE_UNHANDLED_MESSAGE_CHECKS to 0
effectively turns off the checking. If the intention is to replace the reporting mechanism, then
use \ref Theron::Framework::SetFallbackHandler "SetFallbackHandler" to replace the default handler
entirely with a custom handler implementation.

Defaults to 1 (enabled). Set this to 0 to disable the reporting.

The value of \ref THERON_ENABLE_UNHANDLED_MESSAGE_CHECKS can be overridden by defining it
globally in the build (in the makefile using -D, or in the project preprocessor
settings in Visual Studio).
*/
#define THERON_ENABLE_UNHANDLED_MESSAGE_CHECKS 1
#endif // THERON_ENABLE_UNHANDLED_MESSAGE_CHECKS


#ifndef THERON_MAX_THREADS_PER_FRAMEWORK
/**
\brief Hard limit on the maximum number of worker threads software is allowed to enable.

This define sets a hard limit on the number of worker threads each \ref Theron::Framework
"Framework" is allowed to own at once. The actual number of threads in each framework is set in
code; this is just a hard limit on the maximum number that can be set. Requests to create
larger numbers of threads are clamped to the value defined by this define.

Defaults to 1024.

The value of \ref THERON_MAX_THREADS_PER_FRAMEWORK can be overridden by defining it
globally in the build (in the makefile using -D, or in the project preprocessor
settings in Visual Studio).
*/
#define THERON_MAX_THREADS_PER_FRAMEWORK 1024
#endif // THERON_MAX_THREADS_PER_FRAMEWORK


#ifndef THERON_MAX_ACTORS
/**
\brief Limits the maximum number of actors that can be created at once within Theron.

If you want to create a very large number of simultaneous actors then you'll need to
change the value of this define. Note that doing so may lead to higher fixed memory
overheads, so should be avoided unless really necessary.

Defaults to 8192.

The value of \ref THERON_MAX_ACTORS can be overridden by defining it globally in the build
(in the makefile using -D, or in the project preprocessor settings in Visual Studio).
*/
#define THERON_MAX_ACTORS 8192
#endif // THERON_MAX_ACTORS


#ifndef THERON_MAX_RECEIVERS
/**
\brief Limits the maximum number of receivers that can be created at once within Theron.

If you want to create a very large number of simultaneous receivers then you'll need to
change the value of this define. Note that doing so may lead to higher fixed memory
overheads, so should be avoided unless really necessary.

Defaults to 256.

The value of \ref THERON_MAX_RECEIVERS can be overridden by defining it globally in the build
(in the makefile using -D, or in the project preprocessor settings in Visual Studio).
*/
#define THERON_MAX_RECEIVERS 256
#endif // THERON_MAX_RECEIVERS


#endif // THERON_DEFINES_H

