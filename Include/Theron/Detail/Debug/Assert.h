// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_DETAIL_DEBUG_ASSERT_H
#define THERON_DETAIL_DEBUG_ASSERT_H


#include <Theron/Defines.h>

#if THERON_ENABLE_ASSERTS

#include <stdio.h>
#include <assert.h>


#ifndef THERON_ASSERT
#define THERON_ASSERT(condition)                if (!(condition)) Theron::Detail::Fail(__FILE__, __LINE__); else { }
#endif // THERON_ASSERT

#ifndef THERON_ASSERT_MSG
#define THERON_ASSERT_MSG(condition, msg)       if (!(condition)) Theron::Detail::Fail(__FILE__, __LINE__, msg); else { }
#endif // THERON_ASSERT_MSG

#ifndef THERON_FAIL
#define THERON_FAIL()                           Theron::Detail::Fail(__FILE__, __LINE__)
#endif // THERON_FAIL

#ifndef THERON_FAIL_MSG
#define THERON_FAIL_MSG(msg)                    Theron::Detail::Fail(__FILE__, __LINE__, msg)
#endif // THERON_ASSERT

#else

#ifndef THERON_ASSERT
#define THERON_ASSERT(condition)
#endif // THERON_ASSERT

#ifndef THERON_ASSERT_MSG
#define THERON_ASSERT_MSG(condition, msg)
#endif // THERON_ASSERT_MSG

#ifndef THERON_FAIL
#define THERON_FAIL()
#endif // THERON_FAIL

#ifndef THERON_FAIL_MSG
#define THERON_FAIL_MSG(msg)
#endif // THERON_FAIL_MSG

#endif // THERON_ENABLE_ASSERTS



namespace Theron
{
namespace Detail
{


#if THERON_ENABLE_ASSERTS
/**
Reports an internal application or system failure.
A message describing the failure is printed to stderr.
\param file The name of the file in which the failure occurred.
\param line The line number at which the failure occurred.
\param message A message describing the failure.
*/
inline void Fail(const char *const file, const unsigned int line, const char *const message = 0)
{
    fprintf(stderr, "FAIL in %s (%d)", file, line);
    if (message)
    {
        fprintf(stderr, ": %s", message);
    }

    fprintf(stderr, "\n");
    assert(false);
}
#endif // THERON_ENABLE_ASSERTS


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_DEBUG_ASSERT_H

