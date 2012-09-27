// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_DETAIL_NETWORK_NAMEGENERATOR_H
#define THERON_DETAIL_NETWORK_NAMEGENERATOR_H


#include <stdlib.h>

#include <Theron/Assert.h>
#include <Theron/BasicTypes.h>
#include <Theron/Defines.h>

#include <Theron/Detail/Network/String.h>


#ifdef _MSC_VER
#pragma warning(push)
#pragma warning (disable:4996)  // function or variable may be unsafe.
#endif //_MSC_VER


namespace Theron
{
namespace Detail
{


/**
Generates string names from numbers.
*/
class NameGenerator
{
public:

    inline static String Generate(const uint32_t id)
    {
        char buffer[9];
        sprintf(buffer, "%x", id);
        return String(buffer);
    }

    inline static String Combine(
        const char *const rawName,
        const char *const frameworkName,
        const char *const networkName)
    {
        // Scope the mailbox name with the framework name.
        const char *parts[3] = { 0 };
        String result(rawName);

        if (frameworkName)
        {
            parts[0] = result.GetValue();
            parts[1] = ".";
            parts[2] = frameworkName;

            result = String(parts, 3);
        }

        if (networkName)
        {
            parts[0] = result.GetValue();
            parts[1] = ".";
            parts[2] = networkName;

            result = String(parts, 3);
        }

        return result;
    }
};


} // namespace Detail
} // namespace Theron


#ifdef _MSC_VER
#pragma warning(pop)
#endif //_MSC_VER


#endif // THERON_DETAIL_NETWORK_NAMEGENERATOR_H
