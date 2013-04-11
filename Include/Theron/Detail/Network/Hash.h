// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_DETAIL_NETWORK_HASH_H
#define THERON_DETAIL_NETWORK_HASH_H


#include <Theron/Assert.h>
#include <Theron/BasicTypes.h>
#include <Theron/Defines.h>

#include <Theron/Detail/Network/String.h>


namespace Theron
{
namespace Detail
{


/**
\brief Simple hash utility for strings.
*/
class Hash
{
public:

    enum
    {
        RANGE = 256
    };

    THERON_FORCEINLINE static uint32_t Compute(const String &name)
    {
        THERON_ASSERT(!name.IsNull());

        // XOR the first n characters of the string together.
        const char *const str(name.GetValue());
        const char *const end(str + 64);

        const char *ch(str);
        uint8_t hash(0);

        while (ch != end && *ch != '\0')
        {
            hash ^= static_cast<uint8_t>(*ch);
            ++ch;
        }

        return static_cast<uint32_t>(hash);
    }
};


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_NETWORK_HASH_H
