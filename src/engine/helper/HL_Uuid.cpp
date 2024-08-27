#include "HL_Uuid.h"
#include <memory.h>
#include <random>
#include <span>

#define UUID_SYSTEM_GENERATOR
#include "uuid.h"

Uuid::Uuid()
{
    memset(&mUuid, 0, sizeof mUuid);
}

Uuid Uuid::generateOne()
{
    Uuid result;

    auto id = uuids::uuid_system_generator{}();
    memcpy(result.mUuid, id.as_bytes().data(), id.as_bytes().size_bytes());
    return result;
}

Uuid Uuid::parse(const std::string &s)
{
    Uuid result;
    auto id = uuids::uuid::from_string(s);
    if (id)
        memcpy(result.mUuid, id->as_bytes().data(), id->as_bytes().size_bytes());

    return result;
}

std::string Uuid::toString() const
{
    auto s = std::span<uuids::uuid::value_type, 16>{(uuids::uuid::value_type*)mUuid, 16};
    uuids::uuid id(s);
    return uuids::to_string(id);
}

bool Uuid::operator < (const Uuid& right) const
{
    return memcmp(mUuid, right.mUuid, sizeof(mUuid)) < 0;
    return false;
}
