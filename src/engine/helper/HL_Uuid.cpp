#include "HL_Uuid.h"
#include <memory.h>

Uuid::Uuid()
{
#if defined(TARGET_WIN) || defined(TARGET_LINUX) || defined(TARGET_OSX)
    memset(&mUuid, 0, sizeof mUuid);
#endif
}

Uuid Uuid::generateOne()
{
    Uuid result;
#if !defined(USE_NULL_UUID)
#if defined(TARGET_LINUX) || defined(TARGET_OSX)
    uuid_generate(result.mUuid);
#endif
#if defined(TARGET_WIN)
    UuidCreate(&result.mUuid);
#endif
#endif
    return result;
}

Uuid Uuid::parse(const std::string &s)
{
    Uuid result;
#if !defined(USE_NULL_UUID)
#if defined(TARGET_LINUX) || defined(TARGET_OSX)
    uuid_parse(s.c_str(), result.mUuid);
#endif

#if defined(TARGET_WIN)
    UuidFromStringA((RPC_CSTR)s.c_str(), &result.mUuid);
#endif
#endif
    return result;
}

std::string Uuid::toString() const
{
    char buf[64];
#if defined(USE_NULL_UUID)
    return "UUID_disabled";
#else
#if defined(TARGET_LINUX) || defined(TARGET_OSX)
    uuid_unparse_lower(mUuid, buf);
#endif

#if defined(TARGET_WIN)
    RPC_CSTR s = nullptr;
    UuidToStringA(&mUuid, &s);
    if (s)
    {
        strcpy(buf, (const char*)s);
        RpcStringFreeA(&s);
        s = nullptr;
    }
#endif

    return buf;
#endif
}

bool Uuid::operator < (const Uuid& right) const
{
#if defined(TARGET_LINUX) || defined(TARGET_OSX)
    return memcmp(mUuid, right.mUuid, sizeof(mUuid)) < 0;
#endif

#if defined(TARGET_WIN)
    return memcmp(&mUuid, &right.mUuid, sizeof(mUuid)) < 0;
#endif

    return false;
}
