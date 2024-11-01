#include "HL_Uuid.h"
#include <memory.h>
#include <random>
#include <span>

#if defined(TARGET_LINUX)
#define UUID_SYSTEM_GENERATOR
#endif

#include "uuid.h"

Uuid::Uuid()
{
    memset(&mUuid, 0, sizeof mUuid);
}

Uuid Uuid::generateOne()
{
    Uuid result;
#if defined(TARGET_LINUX)
    auto id = uuids::uuid_system_generator{}();
#else
    std::random_device rd;
    auto seed_data = std::array<int, std::mt19937::state_size> {};
    std::generate(std::begin(seed_data), std::end(seed_data), std::ref(rd));
    std::seed_seq seq(std::begin(seed_data), std::end(seed_data));
    std::mt19937 generator(seq);
    uuids::uuid_random_generator gen{generator};

    auto id = gen();
#endif
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
