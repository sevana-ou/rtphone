#include "HL_Uuid.h"
#include <memory.h>
#include <random>
#include "uuid_v4.h"

Uuid::Uuid()
{
    memset(&mUuid, 0, sizeof mUuid);
}

Uuid Uuid::generateOne()
{
    Uuid result;
#if !defined(USE_NULL_UUID)
    UUIDv4::UUIDGenerator<std::mt19937_64> generatorUUID;
    auto r = generatorUUID.getUUID();
    r.bytes((char*)result.mUuid);
#endif
    return result;
}

Uuid Uuid::parse(const std::string &s)
{
    Uuid result;
    UUIDv4::UUID load;
    load.fromStr(s.c_str());
    load.bytes((char*)result.mUuid);
    return result;
}

std::string Uuid::toString() const
{
    UUIDv4::UUID load((const uint8_t*)mUuid);
    return load.str();
}

bool Uuid::operator < (const Uuid& right) const
{
    return memcmp(mUuid, right.mUuid, sizeof(mUuid)) < 0;
    return false;
}
