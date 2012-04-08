// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_DETAIL_THREADING_LOCK_H
#define THERON_DETAIL_THREADING_LOCK_H


#include <Theron/Defines.h>


#ifndef THERON_USE_BOOST_THREADS
#error "THERON_USE_BOOST_THREADS is not defined"
#endif // THERON_USE_BOOST_THREADS

#ifndef THERON_USE_STD_THREADS
#error "THERON_USE_STD_THREADS is not defined"
#endif // THERON_USE_STD_THREADS

#if THERON_USE_BOOST_THREADS
#include <Theron/Detail/Threading/Boost/Lock.h>
#elif THERON_USE_STD_THREADS
#include <Theron/Detail/Threading/Std/Lock.h>
#else
#include <Theron/Detail/Threading/Win32/Lock.h>
#endif // THERON_USE_BOOST_THREADS


#endif // THERON_DETAIL_THREADING_LOCK_H

