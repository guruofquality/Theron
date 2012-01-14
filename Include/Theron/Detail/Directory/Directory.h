// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_DETAIL_DIRECTORY_DIRECTORY_H
#define THERON_DETAIL_DIRECTORY_DIRECTORY_H


#include <Theron/Detail/Threading/Mutex.h>

#include <Theron/Defines.h>


namespace Theron
{
namespace Detail
{


/// Provides thread synchronization for the actor pool and receiver registry.
class Directory
{
public:

    /// Gets a reference to the mutex which locks the directory singleton for exclusive thread access.
    inline static Mutex &GetMutex();

private:

    Directory();
    Directory(const Directory &other);
    Directory &operator=(const Directory &other);

    static Mutex smMutex;                   ///< Mutex that protects access to the instance.
};


THERON_FORCEINLINE Mutex &Directory::GetMutex()
{
    return smMutex;
}


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_DIRECTORY_DIRECTORY_H

