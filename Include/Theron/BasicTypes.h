// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_BASICTYPES_H
#define THERON_BASICTYPES_H


#if defined(_MSC_VER)
// uintptr_t is already defined in Visual C++ builds.
#elif defined(__GNUC__)
// This header defines the uintptr_t type in gcc builds.
// The header is defined in C99, and some modern compilers supply it.
#include <inttypes.h>
#else
// We rely on the typedef within the Theron namespace defined below.
#endif // !defined(_MSC_VER)


namespace Theron
{


/// An 8-bit unsigned integer.
typedef unsigned char uint8_t;

/// A 32-bit unsigned integer.
typedef unsigned int uint32_t;

/// A 64-bit unsigned integer.
typedef unsigned long long uint64_t;

#if defined(__LP64__)
// LP64 machine, OS X or Linux
typedef unsigned long long uintptr_t;
#elif defined(_WIN64)
// LLP64 machine, Windows
typedef unsigned long long uintptr_t;
#else
// 32-bit machine, Windows or Linux or OS X
typedef unsigned long uintptr_t;
#endif


}


#endif // THERON_BASICTYPES_H

