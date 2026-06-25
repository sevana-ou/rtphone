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

    // A fully-seeded Mersenne Twister, built ONCE per thread. The previous code
    // re-seeded a fresh mt19937 from std::random_device on every call: filling
    // mt19937::state_size (624) words via std::random_device means ~624 RDSEED
    // instructions per UUID. Since a UUID is minted for every new stream and streams
    // churn constantly under load, that dominated decode-thread CPU. Seeding once per
    // thread keeps the identical seed quality but turns each UUID into a cheap engine
    // advance. thread_local => each thread owns its engine, so no sharing / no locks.
    static thread_local std::mt19937 generator = []
    {
        std::random_device rd;
        auto seed_data = std::array<int, std::mt19937::state_size> {};
        std::generate(std::begin(seed_data), std::end(seed_data), std::ref(rd));
        std::seed_seq seq(std::begin(seed_data), std::end(seed_data));
        return std::mt19937(seq);
    }();

    uuids::uuid_random_generator gen{generator};
    auto id = gen();
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
